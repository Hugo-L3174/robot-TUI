#include "terminal.h"

using namespace ftxui;

TerminalApp::TerminalApp(Interface & interface) : interface_(interface), interfaceIndex_(0), selectedAddress_(0)
{
  constexpr auto connectionNamesArray = magic_enum::enum_names<ConnectionType>();
  std::vector<std::string> connectionNames{connectionNamesArray.begin(), connectionNamesArray.end()};

  connectionSelector_ = Radiobox(&connectionNames, &interfaceIndex_);

  // @todo for now only getting serial type of connection
  // should be generic function depending on selected interfaceIndex_
  addressSelector_ = Dropdown(&interface_.getSerialPorts(), &selectedAddress_);

  connectButton_ = Button("Connect",
                          [&]
                          {
                            if(interfaceIndex_ == 0)
                            {
                              interface_.connectionType_ = ConnectionType::Serial;
                              interface_.connect(interface_.getSerialPorts()[selectedAddress_]);
                            }
                            else
                            {
                              // @todo
                            }
                          });

  disconnectButton_ = Button("Disconnect", [&] { interface_.disconnect(); });

  searchButton_ = Button("Search", [&] { interface_.detectServos(); });
}
