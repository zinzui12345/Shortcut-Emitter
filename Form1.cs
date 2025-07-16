using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

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
        private int hotkeyId = 0;

        // Handle for the form
        public MainForm()
        {
            InitializeComponent();
            this.Load += Form1_Load;
            this.FormClosing += MainForm_FormClosing;
        }

        // Method to register a hotkey when the form loads
        private void Form1_Load(object sender, EventArgs e)
        {
            // Example: Register Ctrl + Alt + S as a hotkey
            hotkeyId = 1; // Unique ID for the hotkey

            // Register the hotkey
            if (!RegisterHotKey(this.Handle, hotkeyId, MOD_CONTROL | MOD_ALT, (int)Keys.S))
            {
                MessageBox.Show("Failed to register hotkey. Error: " + Marshal.GetLastWin32Error());
            }
        }

        // Method to handle the hotkey press
        protected override void WndProc(ref Message m)
        {
            if (m.Msg == WM_HOTKEY && m.WParam.ToInt32() == hotkeyId)
            {
                // Hotkey pressed, perform the desired action
                MessageBox.Show("Hotkey Ctrl + Alt + S pressed!");

                // You can add your custom logic here
            }
            base.WndProc(ref m);
        }

        // Method to unregister the hotkey when the form is closing
        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Unregister the hotkey
            if (!UnregisterHotKey(this.Handle, hotkeyId))
            {
                MessageBox.Show("Failed to unregister hotkey. Error: " + Marshal.GetLastWin32Error());
            }
        }
    }
}
