FROM ros:lyrical-ros-base

RUN apt-get update \
    && apt-get install -y libgpiod-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ws