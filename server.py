import socket
import threading
import pyaudio
import tkinter as tk
from tkinter import ttk, messagebox

def list_output_devices(p):
    devices = []
    for i in range(p.get_device_count()):
        dev = p.get_device_info_by_index(i)
        if dev["maxOutputChannels"] > 0:
            devices.append((i, dev["name"]))
    return devices

def find_vb_cable_index(p):
    TARGETS = ["cable input", "vb-audio", "cable-vb"]
    for i in range(p.get_device_count()):
        dev = p.get_device_info_by_index(i)
        if dev["maxOutputChannels"] > 0:
            name = dev["name"].lower()
            if any(t in name for t in TARGETS):
                return i
    return None

FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 32730
CHUNK = 1024
SERVER_PORT = 5000

def open_audio_stream(p, rate, device_index):
    return p.open(
        format=FORMAT,
        channels=CHANNELS,
        rate=rate,
        output=True,
        output_device_index=device_index,
        frames_per_buffer=CHUNK
    )

def server_thread(ip, device_index):
    global RATE

    p = pyaudio.PyAudio()
    stream = open_audio_stream(p, RATE, device_index)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((ip, SERVER_PORT))

    print(f"[SERVER] Listening on {ip}:{SERVER_PORT}")

    try:
        while True:
            data, addr = sock.recvfrom(4096)

            if data.startswith(b"SRATE:"):
                try:
                    new_rate = int(data.decode().split(":")[1])
                    if new_rate != RATE:
                        print(f"[INFO] Switching sample rate {RATE} → {new_rate}")

                        stream.stop_stream()
                        stream.close()

                        RATE = new_rate
                        stream = open_audio_stream(p, RATE, device_index)
                        print(f"[SUCCESS] Restarted at {RATE} Hz.")
                except Exception as e:
                    print(f"[ERROR] SRATE parse: {e}")
                continue

            stream.write(data)

    except Exception as e:
        print(f"[SERVER ERROR] {e}")

    finally:
        try:
            stream.stop_stream()
            stream.close()
        except:
            pass

        p.terminate()
        sock.close()
        print("[SERVER] Closed.")

def enable_dark_mode(root):
    root.configure(bg="#1e1e1e")

    style = ttk.Style()
    style.theme_use("clam")

    style.configure(
        "TLabel",
        background="#1e1e1e",
        foreground="white"
    )
    style.configure(
        "TCombobox",
        fieldbackground="#333333",
        background="#444444",
        foreground="white"
    )
    style.configure(
        "TButton",
        background="#333333",
        foreground="white",
        padding=6
    )
    style.map(
        "TButton",
        background=[("active", "#555555")]
    )

root = tk.Tk()
root.title("FridgeMic Server :3")
root.geometry("520x310")

enable_dark_mode(root)
ip_frame = tk.Frame(root, bg="#1e1e1e")
ip_frame.pack(pady=10)

ttk.Label(ip_frame, text="Server IP: ").pack(side=tk.LEFT)

hostname = socket.gethostname()
SERVER_IP = socket.gethostbyname(hostname)

ip_var = tk.StringVar(value=SERVER_IP)
ip_entry = ttk.Entry(ip_frame, textvariable=ip_var, width=30)
ip_entry.pack(side=tk.LEFT)
p = pyaudio.PyAudio()
device_list = list_output_devices(p)

device_frame = tk.Frame(root, bg="#1e1e1e")
device_frame.pack(pady=10)

ttk.Label(device_frame, text="Output Device: ").pack(side=tk.LEFT)

device_var = tk.StringVar()
device_dropdown = ttk.Combobox(device_frame, textvariable=device_var, width=45)

display_values = []
index_map = {}
for idx, name in device_list:
    label = f"{idx}: {name}"
    display_values.append(label)
    index_map[label] = idx

device_dropdown["values"] = display_values
device_dropdown.pack(side=tk.LEFT)
vb_index = find_vb_cable_index(p)
p.terminate()

if vb_index is not None:
    for label, idx in index_map.items():
        if idx == vb_index:
            device_dropdown.set(label)
            break
else:
    device_dropdown.current(0)
def start_server():
    label = device_var.get()

    if label not in index_map:
        messagebox.showerror("Error", "Invalid output device.")
        return

    device_index = index_map[label]
    ip = ip_var.get().strip()

    if not ip:
        messagebox.showerror("Error", "IP cannot be empty.")
        return

    t = threading.Thread(target=server_thread, args=(ip, device_index), daemon=True)
    t.start()

    messagebox.showinfo("Server Running", f"Listening on {ip}:{SERVER_PORT}")

start_btn = ttk.Button(root, text="Start Server", command=start_server)
start_btn.pack(pady=20)

root.mainloop()
