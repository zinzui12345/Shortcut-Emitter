Usage:
```javascript
var shortcut_emitter = ""; // peer id

io.on('connection', (socket) => {
    // Set keys and it's Action
    socket.on('peer_connected', (peer_type) => {
        switch (peer_type) {
            case 'shortcut_emitter':
                if (shortcut_emitter == "") {
                    console.log("set shortcut emitter in " + socket.id);
                    shortcut_emitter = socket.id;
                    io.to(shortcut_emitter).emit('set_shortcut', JSON.stringify([
                        {
                            key:    "Ctrl+Alt+E",
                            method: "speak_next_dialog"
                        },
                        {
                            key:    "Ctrl+Alt+Q",
                            method: "reply_last_comment"
                        }
                    ]));
                }
                break;
        }
    });

    // Handle the Action
    socket.on('peer_request', (function_name, function_parameter=null) => {
        switch (function_name) {
            case 'speak_next_dialog':
                io.to(dialog_manager).emit('speak_next_dialog');
                break;
            case 'reply_last_comment':
                io.to(comment_manager).emit('reply_last_comment');
                break;
        }
    });
});
```
