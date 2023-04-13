#~/bin/bash
export IS_SIMULATION=1
export JACKAL_GX5_IMU=1
export JACKAL_NAVSAT=1
export JACKAL_LASER=1
export JACKAL_BB2=1
export IS_JACKAL=1
roslaunch jackal_gazebo jackal_world.launch
