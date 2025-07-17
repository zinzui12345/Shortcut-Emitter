using SocketIOClient;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Runtime.InteropServices;

namespace Shortcut_Emitter
{
    public partial class MainForm : Form

    {
        // Constant for the modifier key
        private const int MOD_NONE = 0x0000; // No modifier
        private const int MOD_WIN = 0x0008; // Windows key
        private const int MOD_SHIFT = 0x0004;
        private const int MOD_ALT = 0x0001;
        private const int MOD_CONTROL = 0x0002;

        // Constant for Windows messages
        private const int WM_HOTKEY = 0x0312;

        // RegisterHotKey function to register a hotkey
        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool RegisterHotKey(IntPtr hWnd, int id, int fsModifiers, int vk);

        // UnregisterHotKey function to unregister a hotkey
        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool UnregisterHotKey(IntPtr hWnd, int id);

        // Handle for the hotkey ID
        private Dictionary<int, Action> hotkeys = new Dictionary<int, Action>();
        private int hotkeyId;

        // Constructor to initialize Socket IO
        private SocketIOClient.SocketIO socketIO;
        private String serverIP = "https://localhost:3000"; // Default server IP

        // Handle for the form
        private bool isSocketConnected = false;
        public MainForm()
        {
            InitializeComponent();
            this.Load += Form1_Load;
            this.FormClosing += MainForm_FormClosing;
            this.FormBorderStyle = FormBorderStyle.FixedSingle;
        }
        public void setUIConnected(bool connected)
        {
            if (InvokeRequired)
            {
                Invoke(new Action<bool>(setUIConnected), connected);
                return;
            }
            if (connected)
            {
                toolStripStatusLabel1.Text = "Connected";
                toolStripStatusLabel1.ForeColor = Color.Green;
                socketIP.Enabled = false;
                connectButton.Text = "disconnect";
            }
            else
            {
                toolStripStatusLabel1.Text = "Disconnected";
                toolStripStatusLabel1.ForeColor = Color.Red;
                socketIP.Enabled = true;
                connectButton.Text = "connect";

            }
        }
        public void addListBoxValue(string value)
        {
            if (InvokeRequired)
            {
                Invoke(new Action<string>(addListBoxValue), value);
                return;
            }
            listBox1.Items.Add(value);
        }
        public void clearListBoxValue()
        {
            if (InvokeRequired)
            {
                Invoke(new Action(clearListBoxValue));
                return;
            }
            listBox1.Items.Clear();
        }

        // Method to register a custom hotkey
        private void RegisterMyHotkey(int id, int modifiers, Keys key, Action actionToExecute)
        {
            if (RegisterHotKey(this.Handle, id, modifiers, (int)key))
            {
                hotkeys.Add(id, actionToExecute);
                hotkeyId = id; // Store the last registered hotkey ID
            }
            else
            {
                MessageBox.Show("Failed to register hotkey. Error: " + Marshal.GetLastWin32Error());
            }
        }

        // Method that get called when the form loads
        private void Form1_Load(object sender, EventArgs e)
        {
            // do something here
        }

        // Method to handle the hotkey press
        protected override void WndProc(ref Message m)
        {
            if (m.Msg == WM_HOTKEY && hotkeys.ContainsKey(m.WParam.ToInt32()))
            {
                // Hotkey pressed, perform the desired action
                hotkeys[m.WParam.ToInt32()].Invoke();
            }
            base.WndProc(ref m);
        }

        // Method to unregister the hotkey when the form is closing
        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Unregister all the hotkey
            foreach (var id in hotkeys.Keys)
            {
                UnregisterHotKey(this.Handle, id);
            }
        }

        // Method to set up the Socket.IO IP
        private void socketIP_TextChanged(object sender, EventArgs e)
        {
            serverIP = socketIP.Text.Trim();
        }

        // Method to Connect to the Socket.IO server
        private void ConnectSocket()
        {
            if (socketIO != null && socketIO.Connected)
            {
                // If already connected, disconnect
                DisconnectSocket();
            }
            else
            {
                // If not connected, connect to the server
                System.Diagnostics.Debug.WriteLine("Connecting to " + "https://" + serverIP + ":3000");
                socketIO = new SocketIOClient.SocketIO("https://" + serverIP + ":3000", new SocketIOOptions
                {
                    Reconnection = true,
                    ReconnectionAttempts = 5,
                    Transport = SocketIOClient.Transport.TransportProtocol.WebSocket,
                    RemoteCertificateValidationCallback = (_, _, _, _) =>
                    {
                        return true;
                    }
                });

                socketIO.On("set_shortcut", response =>
                {
                    // Get the shortcut data from the response
                    var shortcutData = response.GetValue<string>(0);

                    // Parse the data using Newtonsoft.Json
                    dynamic data = JsonConvert.DeserializeObject(shortcutData);

                    if (data.Type is Newtonsoft.Json.Linq.JTokenType.Array)
                    {
                        // Clear ListBox value
                        clearListBoxValue();

                        // Loop the hotkey data
                        for (int i = 0; i < data.Count; i++)
                        {
                            // Assuming data[i] has properties 'key' and 'method'
                            // MessageBox.Show(data[i].key.Value + " => " + data[i].method.Value);

                            // Display Hotkey in ListBox
                            addListBoxValue(data[i].key.Value + " => " + data[i].method.Value);
                            
                            // Unregister the hotkey if it already exists
                            if (this.InvokeRequired)
                            {
                                this.Invoke(new Action(() => {
                                    if (hotkeys.ContainsKey(i + 1))
                                    {
                                        UnregisterHotKey(this.Handle, i + 1);
                                        hotkeys.Remove(i + 1);
                                    }
                                }));
                            }
                            else
                            {
                                if (hotkeys.ContainsKey(i + 1))
                                {
                                    UnregisterHotKey(this.Handle, i + 1);
                                    hotkeys.Remove(i + 1);
                                }
                            }
                            
                            // data[i].key.Value is "Ctrl+Shift+S" so split it to get the keys
                            string[] keys = data[i].key.Value.Split('+');

                            // Parse the keys and modifiers
                            int modifiers = MOD_NONE;
                            Keys key = Keys.None;
                            foreach (var k in keys)
                            {
                                switch (k.Trim().ToLower())
                                {
                                    case "ctrl":
                                        modifiers |= MOD_CONTROL;
                                        break;
                                    case "shift":
                                        modifiers |= MOD_SHIFT;
                                        break;
                                    case "alt":
                                        modifiers |= MOD_ALT;
                                        break;
                                    case "win":
                                        modifiers |= MOD_WIN;
                                        break;
                                    default:
                                        // Parse the key as a Keys enum
                                        if (Enum.TryParse(k.Trim(), true, out Keys parsedKey))
                                        {
                                            key = parsedKey;
                                        }
                                        else
                                        {
                                            MessageBox.Show("Invalid key: " + k);
                                            continue; // Skip to the next iteration
                                        }
                                        break;
                                }
                            }
                            string pressedHotKey = data[i].key.Value;
                            string methodToCall = data[i].method.Value;
                            var actionToExecute = () =>
                            {
                                // Action to perform when the hotkey is pressed
                                // MessageBox.Show("Hotkey [" + i.ToString()  + "] " + pressedHotKey + " pressed, executing method: " + methodToCall);

                                // Emit the method to the server
                                socketIO.EmitAsync("peer_request", methodToCall);
                            };

                            // Register the hotkey with the parsed modifiers and key
                            if (this.InvokeRequired)
                            {
                                this.Invoke(new Action(() => {
                                    RegisterMyHotkey(i + 1, modifiers, key, actionToExecute);
                                }));
                                continue;
                            }
                            RegisterMyHotkey(i + 1, modifiers, key, actionToExecute);
                        }
                    }
                    else
                    {
                        MessageBox.Show("json data type is not Array!");
                    }

                    // The socket.io server code looks like this:
                    // socket.emit('hi', 'ok', { id: 1, name: 'tom'});
                });

                socketIO.OnConnected += async (sender, e) =>
                {
                    await socketIO.EmitAsync("peer_connected", "shortcut_emitter");

                    MessageBox.Show("Connected to " + serverIP);
                    isSocketConnected = true;
                    setUIConnected(true);

                    // Emit a string and an object
                    //var dto = new TestDTO { Id = 123, Name = "bob" };
                    //await socketIO.EmitAsync("register", "source", dto);
                };
                socketIO.OnDisconnected += (sender, e) =>
                {
                    MessageBox.Show("Disconnected from " + serverIP);
                    isSocketConnected = false;
                    setUIConnected(false);
                };

                // Connect to the Socket.IO server
                syncConnectionStatus();
            }
        }

        // Method to disconnect from the Socket.IO server
        private void DisconnectSocket()
        {
            if (socketIO != null && socketIO.Connected)
            {
                socketIO.DisconnectAsync();
                isSocketConnected = false;
                setUIConnected(false);
            }
        }

        // Method to connect to the Socket.IO server when the button is clicked
        private void connectButton_Click(object sender, EventArgs e)
        {
            if (isSocketConnected)
            {
                // If already connected, disconnect
                DisconnectSocket();
            }
            else
            {
                // If not connected, connect to the server
                ConnectSocket();
            }
        }

        // Method to sync Socket.IO connection status
        private async void syncConnectionStatus()
        {
            // Wait for the connection to be established
            await socketIO.ConnectAsync();
        }
    }
}
