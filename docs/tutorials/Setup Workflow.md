# Step-by-Step Setup Workflow

A checklist for going from an empty repo to "simulates on the Mac, runs on the Pi." For the reasoning behind each choice, see `Project Setup Guide.md`.

**Notes on the links**
- ROS 2 Lyrical's own docs site wasn't live at `docs.ros.org/en/lyrical/` when this was written, so the ROS tutorial links point at **Jazzy** (`docs.ros.org/en/jazzy/`). The tutorials are nearly identical across releases. Try swapping `jazzy` for `lyrical` in the URL later.
- The official Docker image `ros:lyrical-ros-base` **does exist** (Docker Hub tag list checked on 2026-10-30, including an Ubuntu "resolute" variant).
- The Raspberry Pi docs pages returned an error to our automated link check (likely bot blocking), but they are the official URLs and should open in a browser.

Each step ends with a **Done when** line. Don't move on until it's true.

---

## Stage 0: Everyone gets the basics

### Step 0.1: Git and the shared repo
- Each teammate clones `https://github.com/samjackson24816/Olin-FUN-Machine.git` and can push and pull.
- Agree on a simple rule: small commits, pull before you push, one person edits a given file at a time when possible.
- Tutorial: [Pro Git book, Getting Started](https://git-scm.com/book/en/v2/Getting-Started-What-is-Git%3F)
- **Done when:** a teammate pushes a change and you can pull it.

### Step 0.2: Learn Docker enough to be useful
- Install Docker Desktop on the Mac. Learn images vs. containers, `docker run`, volumes (bind mounts), and `docker compose`.
- Tutorials:
  - [Docker Get Started](https://docs.docker.com/get-started/)
  - [Docker Compose Getting Started](https://docs.docker.com/compose/gettingstarted/)
  - [Compose profiles](https://docs.docker.com/compose/how-tos/profiles/) (this is how we'll switch between `sim` and `hw`)
- **Done when:** `docker run hello-world` works and you can explain what a bind mount does.

---

## Stage 1: ROS 2 running in Docker on the Mac (README phase 1)

### Step 1.1: Pull the ROS image and poke around
```bash
docker run -it --rm ros:lyrical-ros-base bash
# inside:
ros2 --help
```
- Image page: [Docker Hub, ros](https://hub.docker.com/_/ros)
- Check the CPU architecture is arm64 (`uname -m` inside the container).
- **Done when:** `ros2 topic list` runs inside the container.

### Step 1.2: Do the ROS CLI tutorials (inside the container)
- [Beginner: CLI tools](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools.html). Do at least the nodes, topics, and parameters pages.
- Run the `turtlesim`-free parts if the GUI doesn't work in Docker. Use `ros2 run demo_nodes_cpp talker` and `listener` in two terminals instead (`docker exec -it <container> bash` opens a second one).
- Reference for how containers talk: [Run 2 nodes in single or separate Docker containers](https://docs.ros.org/en/jazzy/How-To-Guides/Run-2-nodes-in-single-or-separate-docker-containers.html)
- **Done when:** you've seen a talker's messages arrive at a listener and used `ros2 topic echo`.

### Step 1.3: Write our Dockerfile and compose file
Create `docker/Dockerfile` and `docker/compose.yml`, using the sketches in `Project Setup Guide.md`, section 5. Start from `ros:lyrical-ros-base`. Add `python3-colcon-common-extensions`. Define `sim` and `hw` profiles. Mount `src/` into `/ws/src`.
- **Done when:** `docker compose -f docker/compose.yml --profile sim run --rm sim bash` drops you into a shell with ROS available.

### Step 1.4: Create the workspace and the (empty) packages
- Tutorials:
  - [Creating a workspace](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Creating-A-Workspace/Creating-A-Workspace.html)
  - [Creating your first package](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Creating-Your-First-ROS2-Package.html)
- Make the packages: `fun_core`, `fun_hardware`, `fun_bringup` (add `fun_interfaces` only if needed). Use Python packages (`--build-type ament_python`).
- Add `.gitignore` entries for `build/`, `install/`, `log/`.
- **Done when:** `colcon build --symlink-install` succeeds inside the container and all your packages appear in `ros2 pkg list`.

**Phase 1 checkpoint:** commit and push. Everyone on the team can build the container and the workspace.

---

## Stage 2: Define the nodes and topics (README phase 2)

### Step 2.1: Learn publishers and subscribers
- Tutorial: [Writing a simple publisher and subscriber (Python)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Publisher-And-Subscriber.html)
- Background: [Understanding topics](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.html)
- **Done when:** you've written a talker/listener inside `fun_core` that works.

### Step 2.2: Write down the topic list
Turn the architecture diagram into a table in the repo (a `docs/topics.md` works): topic name, direction, message type, meaning. From the diagram:

| Topic | From → To | Suggested type |
|---|---|---|
| `Big_Lever_State` | Lever node → Decision maker | `std_msgs/Bool` |
| `Falling_Off_Lever_State` | Falling-off lever node → Decision maker | `std_msgs/Bool` |
| `Power_Button_State` | Power button node → Decision maker | `std_msgs/Bool` |
| `Power_Button_Lit` | Decision maker → Power button node | `std_msgs/Bool` |
| `Voice_Line_String` | Decision maker → Voice node | `std_msgs/String` |
| `Multi_Func_Color` | Decision maker → Multi-function node | `std_msgs/ColorRGBA` |
| `Multi_Func_State` | Multi-function node → Decision maker | `std_msgs/Bool` |
| `Display_Image` | Decision maker → Display node | `sensor_msgs/Image` |

ROS topic names are conventionally lowercase with underscores (`big_lever_state`). Pick one style and keep it.
- **Done when:** the team agrees on the table. Changing it later is expensive.

### Step 2.3: Decide the backend structure
Write the abstract base classes (`LeverBackend`, etc.) and the node skeletons that load a backend from a `backend` parameter. See `Project Setup Guide.md`, section 4.
- Tutorial for parameters: [Using parameters in a class (Python)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Using-Parameters-In-A-Class-Python.html)
- **Done when:** every node in the diagram exists as a skeleton that starts and stops cleanly.

---

## Stage 3: Simulated implementations on the Mac (README phase 3)

### Step 3.1: Build the sim backends
One `Sim*` class per component. Start with the lever and the decision maker, then add the rest.
- **Done when:** you can flip a simulated lever and see `Big_Lever_State` change with `ros2 topic echo`.

### Step 3.2: Write `sim.launch.py`
- Tutorial: [Creating launch files](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Launch/Creating-Launch-Files.html)
- Put it in `fun_bringup`. It starts the decision maker plus all nodes with `backend:=sim`.
- **Done when:** `docker compose --profile sim up` starts the whole system.

### Step 3.3: Make a way to poke the inputs
Pick one: CLI `ros2 topic pub`, a keyboard-controlled node, or a small web page that publishes topics. A web page can later double as the Pi's feedback page from `Project Decisions.md`.
- **Done when:** a teammate who didn't write the code can trigger a full interaction through the sim.

### Step 3.4: Write tests for the branching logic
- Plain Python tests first: [pytest getting started](https://docs.pytest.org/en/stable/getting-started.html)
- ROS-level tests later: [ROS 2 testing tutorials](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Testing/Testing-Main.html)
- **Done when:** `colcon test` (or `pytest`) passes and you can add a test for a new branch in a few minutes.

**Phase 3 checkpoint:** the whole decision maker is testable with no hardware. Commit and push.

---

## Stage 4: Raspberry Pi setup (can start in parallel with Stage 3)

### Step 4.1: Flash the Pi
- Use Raspberry Pi Imager with a **64-bit** OS. Set hostname, user, Wi-Fi, and enable SSH in the imager's settings.
- Docs: [Raspberry Pi getting started](https://www.raspberrypi.com/documentation/computers/getting-started.html) and [Raspberry Pi Imager](https://www.raspberrypi.com/software/)
- **Done when:** `ssh <user>@<hostname>.local` works from the Mac.

### Step 4.2: Install Docker on the Pi
- Follow the official steps for your OS: [Debian / Raspberry Pi OS](https://docs.docker.com/engine/install/debian/) or [Ubuntu](https://docs.docker.com/engine/install/ubuntu/).
- Add your user to the `docker` group, then log out and back in.
- **Done when:** `docker run --rm hello-world` works on the Pi.

### Step 4.3: Get the repo and build the same image
```bash
git clone https://github.com/samjackson24816/Olin-FUN-Machine.git
cd Olin-FUN-Machine
docker compose -f docker/compose.yml --profile sim build
```
The first build on a Pi is slow. That's normal.
- Optional editing from the Mac: [VS Code Remote-SSH](https://code.visualstudio.com/docs/remote/ssh)
- **Done when:** the `sim` profile runs on the Pi too. This proves the image is portable before you add any hardware.

---

## Stage 5: Hardware implementations (README phase 4)

Do these **one component at a time**, in this order of increasing difficulty: lever, power button, multi-function button, voice, display.

### Step 5.1: Test the wiring without ROS
- Learn `gpiozero`: [docs](https://gpiozero.readthedocs.io/en/stable/) and [basic recipes](https://gpiozero.readthedocs.io/en/stable/recipes.html)
- Write a 10-line script that prints the Hall-effect sensor value while you move a magnet. Run it on the Pi directly (not in Docker) first.
- **Done when:** you trust the wiring and know which pin and pull-up/down setting works.

### Step 5.2: Put the pin numbers in one place
Create a config file (for example `fun_bringup/config/pins.yaml`) with a name-to-pin map. Use BCM numbering throughout.

### Step 5.3: Write the `Gpio*` backend and give the container access
- Implement the hardware backend with debouncing. Import `gpiozero` *inside* the class.
- Pass the device into the container: `devices: [/dev/gpiochip0]` (or `privileged: true` while prototyping) in the `hw` profile.
- **Done when:** `ros2 run fun_hardware lever_node --ros-args -p backend:=gpio` publishes real values, confirmed with `ros2 topic echo`.

### Step 5.4: Add it to `hw.launch.py`
- Mix real and simulated nodes freely at first.
- **Done when:** `docker compose --profile hw up` runs the real lever alongside simulated everything else.

### Step 5.5: Repeat for the other components
- Outputs (LEDs): GPIO write through `gpiozero`'s `LED` or `RGBLED`.
- Voice: pass `/dev/snd` into the container and play a file to the 3.5mm jack.
- Display: depends on the hardware. Pin that down first (see the open questions in `Project Setup Guide.md`).

**Phase 4 checkpoint:** the full interaction works end to end with real parts.

---

## Stage 6: Ongoing workflow

- **Daily loop:** edit and test in the sim on the Mac, commit and push, `git pull` on the Pi, run the `hw` profile, fix, repeat.
- Test any new decision-maker branch in the sim before touching the Pi.
- Keep notes of anything that worked on the Mac but not on the Pi in the Gotchas section of `Project Setup Guide.md`.

## Troubleshooting quick list

| Symptom | Check |
|---|---|
| `ros2` not found in container | Source the ROS setup (`source /opt/ros/lyrical/setup.bash`), then the workspace (`source install/setup.bash`). |
| Code change not picked up | Did you build with `--symlink-install`? Did you re-source `install/setup.bash`? |
| Nodes can't see each other | Same container, or same Docker network? Mac and Pi won't discover each other by default. |
| GPIO permission denied in container | The device isn't passed in. Check `devices:` or try `privileged: true`. |
| No sound on the Pi | Confirm the 3.5mm output is selected on the Pi and `/dev/snd` is mounted. |
| Random Pi crashes | Suspect the power supply. Use a proper 5V 3A supply. |
