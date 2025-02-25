# psaver

Simple automatic power saver for Linux, written for postmarketos. 

## Building

Install dependencies:

* meson (compile-time dependency)
* wayland

Then run:

    meson build
    ninja -C build
    build/wlr-randr

## Usage
1. Install run-dependencies:
* `sudo`
* `chcpu` (turn off cores)
* `cpupower` (change cpu frequency)
2. First-run to allow run command without sudo:
* `./psaver`
* You will see: `Allow cpupower and chcpu without password... Please, enter password.`
* NOTE: If you screwed up - delete `~/.config/psaver/` path.

3. Change settings
* Inspect cpu freq: `cpupower frequency-info`
![image](https://github.com/user-attachments/assets/84202c8c-ce0e-4b96-8c1b-cf5fe599e67c)
* Take min and max to clipboard (or you can choose another values)
* Open `~/.config/psaver/settings` and change settings:
![image](https://github.com/user-attachments/assets/f7083a74-6b74-4a8e-8e81-9819a5cae129)

4. Just run
* `./psaver`
* Or in background: `nohup ./psaver &`

