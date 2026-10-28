#pragma once

#include <algorithm>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
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

// A single record is the whole message plus the associated severity level
// The severity will determine what tag will be rendered in front of the text
struct LogRecord
{
  std::string message;
  LogLevel level = LogLevel::Info;
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

  // Append a message to the buffer with its own severity level for the tag
  void append(std::string_view message, LogLevel level = LogLevel::Info)
  {
    if(message.empty()) return;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      appendRecord(LogRecord{std::string(message), level});
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
      scrollPosition_ = 1.f;
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
          if(mouse.button == ftxui::Mouse::WheelUp)
          {
            scrollPosition_ = std::max(0.f, scrollPosition_ - scrollStep);
          }
          else
          {
            scrollPosition_ = std::min(1.f, scrollPosition_ + scrollStep);
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
          const float trackHeight = static_cast<float>(std::max(1, box_.y_max - box_.y_min));
          const float fromBottom = static_cast<float>(box_.y_max - mouse.y);
          scrollPosition_ = std::clamp(1.f - fromBottom / trackHeight, 0.f, 1.f);
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
      if(event == ftxui::Event::ArrowUp)
      {
        scrollPosition_ = std::max(0.f, scrollPosition_ - scrollStep);
      }
      else if(event == ftxui::Event::ArrowDown)
      {
        scrollPosition_ = std::min(1.f, scrollPosition_ + scrollStep);
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
    float scrollPosition;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      scrollPosition = scrollPosition_;
      rows.reserve(entries_.size());

      for(const auto & entry : entries_)
      {
        // for each entry add the severity tag in front (will be decorated)
        const std::string tag = "[" + std::string(levelName(entry.level)) + "] ";
        ftxui::Elements wrappedLines;

        // lambda to decide how to append the characters of the string to a rendered row so that we can wrap if it's too
        // long
        auto appendCharacters = [&](ftxui::Elements & characters, std::string_view text, bool decorate)
        {
          // Iterate over the bytes of the input string to decompose into each letter/character
          for(size_t byte = 0; byte < text.size();)
          {
            // Using UTF8 encoding: check lead byte to get how many bytes compose this character
            const auto lead = static_cast<unsigned char>(text[byte]);
            // under 0x80 1byte char, after 2bytes until 0xE0 then 3 until 0xF0 then 4
            const size_t length = lead < 0x80 ? 1 : (lead < 0xE0 ? 2 : (lead < 0xF0 ? 3 : 4));
            auto character = ftxui::text(text.substr(byte, std::min(length, text.size() - byte)));
            if(decorate)
            {
              character = character | ftxui::color(levelColor(entry.level)) | ftxui::bold;
            }
            characters.push_back(std::move(character));
            // skip the rest of the bytes of the one we just added
            byte += length;
          }
        };

        size_t lineStart = 0;
        bool firstLine = true;
        while(lineStart <= entry.message.size())
        {
          // check for a linebreak
          const size_t lineEnd = entry.message.find('\n', lineStart);
          // if no linebreak found then we finish at the end of the message
          const size_t end = (lineEnd == std::string::npos) ? entry.message.size() : lineEnd;
          ftxui::Elements characters;
          // Append tag with its level decoration
          if(firstLine)
          {
            appendCharacters(characters, tag, true);
          }
          // Append the rest of the line before wrapping
          appendCharacters(characters, std::string_view(entry.message).substr(lineStart, end - lineStart), false);
          // Use hflow to wrap ftxui elements automatically (since now each char is an element the wrap can occur
          // anywhere in the string)
          wrappedLines.push_back(ftxui::hflow(std::move(characters)));
          firstLine = false;
          if(lineEnd == std::string::npos)
          {
            break;
          }
          lineStart = lineEnd + 1;
        }
        rows.push_back(ftxui::vbox(std::move(wrappedLines)));
      }
    }

    return ftxui::vbox(std::move(rows)) | ftxui::focusPositionRelative(0.f, scrollPosition) | ftxui::vscroll_indicator
           | ftxui::yframe | ftxui::flex | ftxui::reflect(box_);
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
    entries_.push_back(std::move(record));
    while(entries_.size() > maxLines_)
    {
      entries_.pop_front();
    }
  }

  // scroll unit to divide the total log buffer
  static constexpr float scrollStep = 0.05f;
  const size_t maxLines_;
  std::function<void()> requestRefresh_;
  mutable std::mutex mutex_;
  std::deque<LogRecord> entries_;
  float scrollPosition_ = 1.f;
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
