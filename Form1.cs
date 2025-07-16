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

        // Handle for the form
        public MainForm()
        {
            InitializeComponent();
            this.Load += Form1_Load;
            this.FormClosing += MainForm_FormClosing;
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

        // Method to register a hotkey when the form loads
        private void Form1_Load(object sender, EventArgs e)
        {
            // Example: Register Ctrl + Alt + S as a hotkey
            RegisterMyHotkey(1, MOD_CONTROL | MOD_ALT, Keys.S, () =>
            {
                // Action to perform when the example hotkey is pressed
                MessageBox.Show("Hotkey  Ctrl + Alt + S pressed");

                // You can add your custom logic here
            });

            // Example: Register Ctrl + Shift + A as a hotkey
            RegisterMyHotkey(2, MOD_CONTROL | MOD_SHIFT, Keys.A, () =>
            {
                // Action to perform when the example hotkey is pressed
                MessageBox.Show("Hotkey Ctrl + Shift + A pressed");

                // You can add your custom logic here
            });

            // You can register more hotkeys here if needed
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
    }
}
