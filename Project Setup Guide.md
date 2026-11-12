# Project Setup Guide

ROS 2 Lyrical comes from [RoboStack](https://robostack.github.io) through `pixi`, defined entirely by `pixi.toml` + `pixi.lock`.
Same files and commands on the Raspberry Pi (Debian 13, arm64) and on the Macs (Apple silicon). No root, no Docker needed.

## One-time install of pixi
Download the release for your machine from https://github.com/prefix-dev/pixi/releases/latest
(Pi: `pixi-aarch64-unknown-linux-musl.tar.gz`, Mac: `pixi-aarch64-apple-darwin.tar.gz`),
verify its `.sha256`, and put the `pixi` binary on your PATH (e.g. `~/.pixi/bin`).

## Daily use (from the repo root)
| Command | What it does |
| --- | --- |
| `pixi run build` | `colcon build` all packages in `src/` |
| `pixi run brain` | build, then run the decision maker |
| `pixi run test-brain` | build, then check lever -> `press_the_button` end to end |
| `pixi run clean` | delete `build/ install/ log/` |
| `pixi shell` | shell with ROS on the PATH (then `source install/setup.bash`) |

First install takes ~2 min, first build ~1.5 min on the Pi.

## Sim vs hardware
Nodes talk only over topics, so simulating a part means publishing its topic yourself, e.g.
`ros2 topic pub --once /big_lever_state std_msgs/msg/Bool "{data: true}"`.
Hardware backends (`fun_hardware`) are still to be written.

## Notes
- `pixi.lock` is committed so everyone gets identical package versions. Update with `pixi update`.
- The `docker/` folder is an alternative for Macs only; the Pi does not use Docker.
