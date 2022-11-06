# osk-touch

This is a framebuffer on-screen keyboard proof-of-concept for TTY, specifically intended for Steam Deck. If configured precisely, it might be useful for accessibility. Currently, you can touch the screen to send the keystrokes to the system, like a real keyboard.

 - No extra libraries. Great for using at boot time:
```
# ldd osk_touch
	linux-vdso.so.1 (0x00007ffc49159000)
	libc.so.6 => /usr/lib/libc.so.6 (0x00007fa930b43000)
	/lib64/ld-linux-x86-64.so.2 => /usr/lib64/ld-linux-x86-64.so.2 (0x00007fa930d39000)
```
 - Onscreen keyboard using framebuffer
 - It reads touchscreen and sends keystrokes using uinput

### How to use

- Be sure that `/dev/input/event5` is your real touchscreen. You need it to touch the onscreen keyboard.
- Check which is your framebuffer device (`/dev/fb0`, `/dev/fb1`, etc).
- You need `root` for commands marked with `#`.
- To execute `osk_touch` as `deck` user, you can use `# usermod -aG input,video deck` first, then log out and log in.

```
$ make
# chvt 4
# modprobe uinput
# ./osk_touch &
```

The `deck_keyboard.ppm` is the keyboard image. **It is a screenshot of Valve's keyboard edited with GIMP and exported as raw ppm.**
