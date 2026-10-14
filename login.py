import tkinter as tk
from tkinter import messagebox
from scripts.authentication import Authentication
from game import Game

SERVERIP = "127.0.0.1"
SERVERPORT = 8080
FONT_PATH = "data/images/menuAssets/font.ttf"

class LoginRegister:
    def __init__(self, game):
        self.authentication = Authentication(None, None)
        self.game = game
        self.window = tk.Tk()
        self.window.title("Game Login/Register")
        self.window.geometry("300x200")

    def login_frame(self):
        # Clear the window
        for widget in self.window.winfo_children():
            widget.destroy()
        
        # Add login widgets
        tk.Label(self.window, text= "Login", font=(FONT_PATH, 14)).pack()
        tk.Label(self.window, text = "Username: ").pack()
        username_entry = tk.Entry(self.window)
        username_entry.pack()

        tk.Label(self.window, text="Password").pack()
        password_entry = tk.Entry(self.window, show="*")
        password_entry.pack()

        def login_user():
            username = username_entry.get()
            password = password_entry.get()
            self.authentication = Authentication(username, password)
            data = self.authentication.login(SERVERIP, SERVERPORT)
            if data == "Login Successfully\n":
                self.window.destroy()
                self.game.menu()
            else:
                messagebox.showerror("Error", data)

        
        tk.Button(self.window, text="Login", command=login_user).pack()
        tk.Button(self.window, text="Go to register", command=self.register_frame).pack()
        self.window.mainloop()

    def register_frame(self):
        # Clear the window
        for widget in self.window.winfo_children():
            widget.destroy()

        # Add register widget
        tk.Label(self.window, text="Register", font=(FONT_PATH, 14)).pack()
        tk.Label(self.window, text="Username: ").pack()
        username_entry = tk.Entry(self.window)
        username_entry.pack()

        tk.Label(self.window, text="Password: ").pack()
        password_entry = tk.Entry(self.window, show="*")
        password_entry.pack()

        def register_user():
            username = username_entry.get()
            password = password_entry.get()
            self.authentication = Authentication(username, password)
            data = self.authentication.register(SERVERIP, SERVERPORT)
            if data == "Register Successfully\n":
                messagebox.showinfo("Success", data)
            else:
                messagebox.showerror("Error", data)
        tk.Button(self.window, text="Register", command=register_user).pack()
        tk.Button(self.window, text="Go to login", command=self.login_frame).pack()
        self.window.mainloop()

if __name__ == "__main__":
    login = LoginRegister(FONT_PATH)
    login.login_frame()
