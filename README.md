# shortcut-emitter
A global shortcut/hotkey for WebSocket Client based on Qt-Applications.

## Features
- Can be connected using custom ports
- Support `ws://` and `wss://` WebSocket protocol
- Supports almost all common keys (Depends on Keyboard-Layout)
- Allows direct input of Key/Modifier-Combinations
- Allows usage of native keycodes and modifiers, if needed

## Requirements:
- CMake
- [QHotkey](https://github.com/Skycoder42/QHotkey)
- [qt5-x11extras](https://archlinux.org/packages/extra/x86_64/qt5-x11extras)
- [qt5-websockets](https://archlinux.org/packages/extra/x86_64/qt5-websockets)

## Usage
* Initialize Shortcuts
  ```javascript
    ws.send(JSON.stringify(
        {
            message: "set_shortcut",
            data:
            [
                {
                    key:    "Ctrl+Alt+E",
                    method: "example_shortcut_1"
                },
                {
                    key:    "Alt+1",
                    method: "example_shortcut_2"
                },
                {
                    key:    "Ctrl+Shift+R",
                    method: "example_shortcut_3"
                },
                {
                    key:    "Ctrl+M",
                    method: "example_shortcut_4"
                }
            ],
            client_type: "server"
        }
    ));
  ```
* Set Shortcuts Function
  ```javascript
    ws.on('message', function message(data) {
        try {
            JSON.parse(data);
        } catch (e) {
            console.log('received: %s', data);
            return false;
        }
        let data_object = JSON.parse(data);
        if ('message' in data_object) {
            if (data_object.message == "hotkey_request") {
                if ('data' in data_object) {
                    switch (data_object.data) {
                        case 'example_shortcut_1':
                            console.log("Shortcut 1 pressed!");
                            break;
                        case 'example_shortcut_2':
                            console.log("Shortcut 2 pressed!");
                            break;
                        case 'example_shortcut_3':
                            console.log("Shortcut 3 pressed!");
                            break;
                        case 'example_shortcut_4':
                            console.log("Shortcut 4 pressed!");
                            break;
                    }
                }
            }
        }
    });
  ```
* Add Shortcut
  ```javascript
    ws.send(JSON.stringify(
        {
            message: "add_shortcut",
            data:
            [
                {
                    key:    "Ctrl+K",
                    method: "example_shortcut_5"
                },
                {
                    key:    "Alt+D",
                    method: "example_shortcut_6"
                }
            ],
            client_type: "server"
        }
    ));
  ```
* Remove Shortcut
  ```javascript
    ws.send(JSON.stringify(
        {
            message: "remove_shortcut",
            data:
            [
                {
                    key: "Ctrl+K"
                },
                {
                    key: "Alt+D"
                }
            ],
            client_type: "server"
        }
    ));
  ```
* `wss://` Server Example
  ```javascript
  const express = require('express');
  const app = express();
  const port = 3000;
  const fs = require('fs');
  const https = require('https');
  const server = https.createServer({
      key: fs.readFileSync('./server.key'),
      cert: fs.readFileSync('./server.cert')
  }, app);
  const { WebSocketServer } = require('ws');
  const wss = new WebSocketServer({ server });

  var shortcut_emitter = "";

  wss.on('connection', function connection(ws, req) {
      const ip = req.socket.remoteAddress;
  
      ws.on('error', console.error);
  
      ws.on('message', function message(data) {
          try {
              JSON.parse(data);
          } catch (e) {
              console.log('received: %s', data);
              return false;
          }
          let data_object = JSON.parse(data);
          if ('message' in data_object) {
              if (data_object.message == "connected") {
                  if (shortcut_emitter == "") {
                      console.log("set shortcut emitter in " + ip);
                      shortcut_emitter = ip;
                      ws.send(JSON.stringify(
                          {
                              message: "set_shortcut",
                              data: // set keybind and method here
                              [
                                  {
                                      key:    "Alt+E",
                                      method: "speak_next_dialog"
                                  },
                                  {
                                      key:    "Alt+Q",
                                      method: "reply_last_comment"
                                  },
                                  {
                                      key:    "Alt+R",
                                      method: "replay_talk"
                                  },
                                  {
                                      key:    "Alt+S",
                                      method: "stop_talk"
                                  }
                              ],
                              client_type: "server"
                          }
                      ));
                  }
                  else {
                      console.log("prevent shortcut emitter in " + ip);
                      ws.close(1000, 'Access Denied!');
                  }
              }
              if (data_object.message == "hotkey_request" && shortcut_emitter == ip) {
                  if ('data' in data_object) {
                      switch (data_object.data) {
                          // do something with keybind method here
                          case 'speak_next_dialog':
                              console.log("Alt+E pressed!");
                              break;
                          case 'reply_last_comment':
                              console.log("Alt+Q pressed!");
                              break;
                          case 'replay_talk':
                              console.log("Alt+R pressed!");
                              break;
                          case 'stop_talk':
                              console.log("Alt+S pressed!");
                              break;
                      }
                  }
              }
          }
      });
  
      ws.on('close', function close() {
          if (shortcut_emitter == ip) {
              console.log("shortcut emitter disconnected")
              shortcut_emitter = "";
          }
      });
  });

  server.listen(port, () => {
    const address = server.address();
    console.log(`Server listening on IP '${address.address}' PORT '${address.port}'`);
    console.log(`set websocket on wss://${address.address}:${address.port}`);
  });
  ```
