#pragma once

#include <algorithm>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>

namespace TUI
{

enum class LogLevel
{
  Info,
  Warning,
  Error,
  Critical,
};

// A message can be built from several parts so they can be styled differently
struct LogPart
{
  std::string text;
  // Optional characteristics to decorate
  std::optional<ftxui::Color> foreground;
  std::optional<ftxui::Color> background;
  bool bold = false;
};

// A log record is composed of differently decorated parts
struct LogRecord
{
  std::vector<LogPart> parts;
};

/**
 * @brief Thread-safe FTXUI component for displaying log output
 *
 * Since cout is already used by the tui this acts as a sink and can be added as a component to render the buffered
 * messages
 */
class Logger : public ftxui::ComponentBase
{
public:
  explicit Logger(size_t maxLines = 1000, std::function<void()> requestRefresh = {})
  : maxLines_(maxLines), requestRefresh_(std::move(requestRefresh))
  {
  }

  // Log a raw message with an optional log level decorator
  void append(std::string_view message, LogLevel level = LogLevel::Info)
  {
    if(message.empty()) return;

    LogRecord record;
    record.parts.push_back(
        LogPart{.text = "[" + std::string(levelName(level)) + "] ", .foreground = levelColor(level), .bold = true});
    record.parts.push_back(LogPart{.text = std::string(message)});
    append(std::move(record));
  }

  // Log an already decorated message
  void append(LogRecord record)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      appendRecord(std::move(record));
    }
    // call tui refresh function
    if(requestRefresh_)
    {
      requestRefresh_();
    }
  }

  void clear()
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      entries_.clear();
      scrollOffset_ = 0;
    }
    if(requestRefresh_)
    {
      requestRefresh_();
    }
  }

  bool OnEvent(ftxui::Event event) override
  {
    if(event.is_mouse())
    {
      const auto & mouse = event.mouse();
      // Only consume wheel events that happen in the logger rendered area
      if(!box_.Contain(mouse.x, mouse.y))
      {
        return false;
      }

      if(mouse.button == ftxui::Mouse::WheelUp || mouse.button == ftxui::Mouse::WheelDown)
      {
        {
          std::lock_guard<std::mutex> lock(mutex_);
          // Offset is at zero at the bottom (newest added element) and goes up with older lines
          const size_t maxOffset = entries_.empty() ? 0 : entries_.size() - 1;
          if(mouse.button == ftxui::Mouse::WheelUp)
          {
            scrollOffset_ = std::min(maxOffset, scrollOffset_ + 3);
          }
          else
          {
            scrollOffset_ = scrollOffset_ > 3 ? scrollOffset_ - 3 : 0;
          }
        }
        if(requestRefresh_)
        {
          requestRefresh_();
        }
        return true;
      }

      if(mouse.button == ftxui::Mouse::Left && mouse.motion == ftxui::Mouse::Pressed)
      {
        // Take focus when clicked
        TakeFocus();
        // Move directly to the correct position if clicked on the scrollbar
        if(mouse.x == box_.x_max)
        {
          std::lock_guard<std::mutex> lock(mutex_);
          const int trackHeight = box_.y_max - box_.y_min + 1;
          const size_t maxOffset = entries_.empty() ? 0 : entries_.size() - 1;
          if(trackHeight > 1)
          {
            // Move proportionally to scroll size
            const int fromBottom = box_.y_max - mouse.y;
            scrollOffset_ =
                maxOffset * static_cast<size_t>(std::max(0, fromBottom)) / static_cast<size_t>(trackHeight - 1);
          }
        }
        if(requestRefresh_) requestRefresh_();
        return true;
      }
      return false;
    }

    // If event is not wheel or mouse click and not focused, ignore
    if(!Focused())
    {
      return false;
    }

    // Otherwise process
    {
      std::lock_guard<std::mutex> lock(mutex_);
      const size_t maxOffset = entries_.empty() ? 0 : entries_.size() - 1;
      if(event == ftxui::Event::ArrowUp)
      {
        scrollOffset_ = std::min(maxOffset, scrollOffset_ + 1);
      }
      else if(event == ftxui::Event::ArrowDown)
      {
        scrollOffset_ = scrollOffset_ > 0 ? scrollOffset_ - 1 : 0;
      }
      else
      {
        return false;
      }
    }

    if(requestRefresh_) requestRefresh_();
    return true;
  }

  ftxui::Element OnRender() override
  {
    std::vector<ftxui::Element> rows;
    size_t focusedRow = 0;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      rows.reserve(entries_.size());
      // ftxui::yframe follows a focused row and keeps the view on it
      if(!entries_.empty())
      {
        focusedRow = entries_.size() - 1 - std::min(scrollOffset_, entries_.size() - 1);
      }

      for(size_t i = 0; i < entries_.size(); ++i)
      {
        std::vector<ftxui::Element> parts;
        for(const auto & part : entries_[i])
        {
          // parse text and decorate if needed
          auto element = ftxui::text(part.text);
          if(part.foreground)
          {
            element = element | ftxui::color(*part.foreground);
          }
          if(part.background)
          {
            element = element | ftxui::bgcolor(*part.background);
          }
          if(part.bold)
          {
            element = element | ftxui::bold;
          }
          parts.push_back(std::move(element));
        }
        auto row = ftxui::hbox(std::move(parts));
        // apply focus if this one is the correct offset
        if(i == focusedRow)
        {
          row = row | ftxui::focus;
        }
        rows.push_back(std::move(row));
      }
    }

    // @fixme this should wrap with terminal, see in ftxui examples
    // The reflected box is used for checking if actions happened on this
    return ftxui::vbox(std::move(rows)) | ftxui::vscroll_indicator | ftxui::yframe
           | ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 12) | ftxui::reflect(box_);
  }

  // Making it focusable so that it can be scrolled instead of passing focus to other elements
  bool Focusable() const override
  {
    return true;
  }

private:
  static std::string_view levelName(LogLevel level)
  {
    switch(level)
    {
      case LogLevel::Info:
        return "Info";
      case LogLevel::Warning:
        return "Warn";
      case LogLevel::Error:
        return "Error";
      case LogLevel::Critical:
        return "Crit";
    }
    return "Info";
  }

  static ftxui::Color levelColor(LogLevel level)
  {
    switch(level)
    {
      case LogLevel::Info:
        return ftxui::Color::Blue;
      case LogLevel::Warning:
        return ftxui::Color::Yellow;
      case LogLevel::Error:
        return ftxui::Color::Red;
      case LogLevel::Critical:
        return ftxui::Color::DeepPink1;
    }
    return ftxui::Color::Blue;
  }

  // Break down record into different lines for easier ftxui render
  void appendRecord(LogRecord record)
  {
    std::vector<LogPart> entry;
    bool splitAtNewline = false;
    for(const auto & part : record.parts)
    {
      size_t begin = 0;
      while(begin <= part.text.size())
      {
        // if the part has a newline, break it down into different entries
        const size_t newline = part.text.find('\n', begin);
        const size_t end = (newline == std::string::npos) ? part.text.size() : newline;
        LogPart linePart = part;
        linePart.text = part.text.substr(begin, end - begin);

        // registering this line as an entry
        if(!linePart.text.empty())
        {
          entry.push_back(std::move(linePart));
        }
        // If nothing left after the \n then part is fully processed
        if(newline == std::string::npos)
        {
          break;
        }

        appendEntry(std::move(entry));
        entry.clear();
        splitAtNewline = true;
        begin = newline + 1;
      }
    }

    // If no \n are left but there is still text push the rest
    if(!entry.empty() || !splitAtNewline)
    {
      appendEntry(std::move(entry));
    }
  }

  // Util function to register a processed line
  void appendEntry(std::vector<LogPart> entry)
  {
    // Increment offset with entry if not at the bottom so that the focused line stays in view
    if(scrollOffset_ > 0)
    {
      scrollOffset_++;
    }
    entries_.push_back(std::move(entry));
    while(entries_.size() > maxLines_)
    {
      entries_.pop_front();
    }
    // clamp offset to avoid being negative after popping
    scrollOffset_ = std::min(scrollOffset_, entries_.empty() ? size_t{0} : entries_.size() - 1);
  }

  const size_t maxLines_;
  std::function<void()> requestRefresh_;
  mutable std::mutex mutex_;
  std::deque<std::vector<LogPart>> entries_;
  size_t scrollOffset_ = 0;
  // rendered space of this logger (used to check if events happen on it)
  ftxui::Box box_;
};

// A separate logger sink that routes its output to the given TUI component for display
class LoggerSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
  explicit LoggerSink(std::weak_ptr<Logger> logger) : logger_(std::move(logger)) {}

protected:
  // sink function override to append message to logger component
  void sink_it_(const spdlog::details::log_msg & message) override
  {
    if(const auto logger = logger_.lock())
    {
      logger->append(std::string_view(message.payload.data(), message.payload.size()), toTuiLevel(message.level));
    }
  }

  void flush_() override {}

private:
  static LogLevel toTuiLevel(spdlog::level::level_enum level)
  {
    switch(level)
    {
      case spdlog::level::warn:
        return LogLevel::Warning;
      case spdlog::level::err:
        return LogLevel::Error;
      case spdlog::level::critical:
        return LogLevel::Critical;
      default:
        return LogLevel::Info;
    }
  }

  std::weak_ptr<Logger> logger_;
};

// the returned sink is not registered gobally so that it can be used by the elements it is passed to
inline std::shared_ptr<spdlog::logger> makeLoggerSink(const std::shared_ptr<Logger> & logger)
{
  auto sink = std::make_shared<LoggerSink>(logger);
  auto result = std::make_shared<spdlog::logger>("TUI-logger", std::move(sink));
  result->set_level(spdlog::get_level());
  return result;
}

} // namespace TUI
