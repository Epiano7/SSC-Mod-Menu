"""Native Tk installer, matching the Windows setup's palette and controls."""
import queue
import threading
import traceback
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox

import linux_desktop as desktop
import ssc_installer as core

BG, PANEL, RAISED, BLUE, CYAN, INK, MUTED = '#0f1c2e', '#1b2f49', '#264060', '#1f75d7', '#4dc4ff', '#dce7f7', '#abc0d9'


class CutButton(tk.Canvas):
    def __init__(self, parent, text, command, width=160, height=40):
        super().__init__(parent, width=width, height=height, bg=BG, highlightthickness=0, takefocus=1)
        self.enabled, self.command, self.caption = True, command, text
        self.width, self.height = width, height
        self.bind('<ButtonRelease-1>', self.invoke)
        self.bind('<Return>', self.invoke)
        self.bind('<space>', self.invoke)
        self.bind('<Enter>', lambda e: self.draw(BLUE))
        self.bind('<Leave>', lambda e: self.draw())
        self.bind('<FocusIn>', lambda e: self.draw(BLUE))
        self.bind('<FocusOut>', lambda e: self.draw())
        self.draw()

    def draw(self, fill=RAISED):
        w, h = self.width, self.height
        self.delete('all')
        self.create_polygon(8, 0, w-8, 0, w, 8, w, h-8, w-8, h, 8, h, 0, h-8, 0, 8,
                            fill=fill if self.enabled else PANEL, outline='')
        self.create_text(w/2, h/2, text=self.caption, fill=INK if self.enabled else MUTED, font=('Sans', 10, 'bold'))

    def invoke(self, event=None):
        if self.enabled:
            self.focus_set()
            self.command()
        return 'break'


def create_window(folder):
    window = tk.Tk()
    window.title('SSC Mod Menu — Linux preview')
    window.configure(bg=BG)
    window.geometry('640x365')
    window.resizable(False, False)
    tk.Label(window, text='SSC Mod Menu', font=('Sans', 22, 'bold'), fg=INK, bg=PANEL).place(x=0,y=0,width=640,height=70)
    tk.Frame(window, bg=CYAN).place(x=0,y=69,width=640,height=2)
    tk.Label(window, text='GAME FOLDER', font=('Sans', 9), fg=MUTED, bg=BG).place(x=24,y=88)
    paths = core.discover()
    selected = tk.StringVar(value=str(paths[0]) if paths else '')
    status = tk.StringVar(value='Linux / Proton preview • Exit Steam before installing')
    entry = tk.Entry(window, textvariable=selected, bg=PANEL, fg=INK, insertbackground=INK,
                     relief='flat', font=('Sans', 10))
    entry.place(x=24,y=116,width=468,height=36)
    events = queue.Queue()
    buttons = []
    busy = False

    def button(caption, x, y, width, action):
        btn = CutButton(window, caption, action, width)
        btn.place(x=x, y=y)
        buttons.append(btn)
        return btn

    def browse():
        value = filedialog.askdirectory(parent=window, title='Choose the Skillshot City game folder', initialdir=selected.get() or str(Path.home()))
        if value:
            selected.set(value)

    button('BROWSE', 504, 114, 112, browse)
    tk.Label(window, textvariable=status, fg=CYAN, bg=BG, wraplength=586, justify='left', anchor='nw', font=('Sans', 10)).place(x=24,y=168,width=592,height=72)

    def work(action):
        nonlocal busy
        if busy:
            return
        game = selected.get()
        if action == 'remove' and not messagebox.askyesno('Uninstall', 'Remove the mod and its Steam launch integration? Your settings and custom sounds will stay saved', parent=window):
            return
        output = None
        if action == 'logs':
            output = filedialog.asksaveasfilename(parent=window, title='Export diagnostic logs', initialfile='SSC-Mod-Menu-Logs.zip', defaultextension='.zip', filetypes=[('ZIP archive', '*.zip')])
            if not output:
                return
        busy = True
        entry.configure(state='disabled')
        for btn in buttons:
            btn.enabled = False
            btn.draw()
        status.set('Exporting logs…' if action == 'logs' else 'Working… Please keep Steam closed')
        def execute():
            try:
                if action == 'logs':
                    result = desktop.export_logs(game, output)
                elif action == 'remove':
                    result = desktop.uninstall_desktop(game)
                else:
                    result = desktop.install_desktop(folder, game, repair=action == 'repair')
                events.put((True, result))
            except Exception as error:
                desktop.log(traceback.format_exc())
                events.put((False, str(error)))
        threading.Thread(target=execute, daemon=True).start()

    button('INSTALL / UPDATE', 24, 246, 204, lambda: work('install'))
    button('UNINSTALL', 240, 246, 180, lambda: work('remove'))
    button('CLOSE', 432, 246, 184, lambda: window.destroy() if not busy else None)
    button('REPAIR', 24, 305, 134, lambda: work('repair'))
    button('EXPORT LOGS', 170, 305, 176, lambda: work('logs'))
    window.protocol('WM_DELETE_WINDOW', lambda: None if busy else window.destroy())
    def poll():
        nonlocal busy
        try:
            success, result = events.get_nowait()
        except queue.Empty:
            pass
        else:
            busy = False
            status.set(result)
            entry.configure(state='normal')
            for btn in buttons:
                btn.enabled = True
                btn.draw()
            if not success:
                messagebox.showerror('SSC Mod Menu', result + '\n\nUse Export Logs to save diagnostics', parent=window)
        window.after(100, poll)
    window.after(100, poll)
    return window


def gui(folder):
    desktop.log('Native installer opened')
    create_window(folder).mainloop()
