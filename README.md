# osk-touch

This is a framebuffer onscreen keyboard for TTY, specifically intended for Steam Deck. It might be useful for accessibility. You can touch the screen to send the keystrokes to the system, like a real keyboard.

 - No extra libraries. Great for using at boot time:
```
# ldd osk_touch
	linux-vdso.so.1 (0x00007ffc49159000)
	libc.so.6 => /usr/lib/libc.so.6 (0x00007fa930b43000)
	/lib64/ld-linux-x86-64.so.2 => /usr/lib64/ld-linux-x86-64.so.2 (0x00007fa930d39000)
```
 - Onscreen keyboard using framebuffer
 - It reads touchscreen and sends keystrokes using uinput

The `deck_keyboard.ppm` is the keyboard image. **It is a screenshot of Valve's keyboard edited with GIMP and exported as raw ppm.**

### How to use

- If you are using SteamOS, you need my [SteamOS on-device native unsandboxed toolchain guide](https://gist.github.com/robertkirkman/753922262259486ec417e5ff8b5b924b#prerequisites-for-all-methods-shown-here) or any equivalent C toolchain you prefer that you can get to work with this app.
- Be sure that "`FTS3528:00 2808:1015`" is the name of your touchscreen. That is the name of the Steam Deck LCD's touchscreen and is used by default. DeckHD and Steam Deck OLED support might come to this project eventually.
- Check which is your framebuffer device (`/dev/fb0`, `/dev/fb1`, etc). `/dev/fb0`, the Steam Deck's, is used by default.
- You need `root` for commands marked with `#`.
- If you experience crashing after updating your OS, you need to update your toolchain too and recompile this app.

```
$ make
# chvt 4
# modprobe uinput
# ./osk_touch &
```

### How to disable automatic login, drop to `tty4` on boot, and launch `osk-touch`
> Example focuses on SteamOS. Directions must be adapted where necessary for other distros.

- After compilation, install `osk-touch` to the writable, persistent `/home/` folder:
```
# cp osk_touch /home/
# cp deck_keyboard.ppm /home/
```

- Create a script in `/home/` named `osk-touch.sh` with this content:
```bash
#!/bin/bash
chvt 4
/home/osk_touch
```

- Mark the script executable:
```
# chmod +x /home/osk-touch.sh
```

- Create a service in `/etc/systemd/system/` named `osk-touch.service`:
```
[Unit]
Description=Touchscreen keyboard for Steam Deck TTY

[Service]
Type=simple
Restart=always
RestartSec=5
WorkingDirectory=/home
ExecStart=/home/osk-touch.sh

[Install]
WantedBy=multi-user.target
```

- Disable automatic starting of SDDM, which will disable automatic login, and enable `osk-touch.service`:
```
# systemctl disable sddm
# systemctl enable osk-touch
```
