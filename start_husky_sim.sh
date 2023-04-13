# Sensor arch
export HUSKY_SENSOR_ARCH=1
export HUSKY_SENSOR_ARCH_HEIGHT='300'

# Velodyne Laser
export HUSKY_LMS1XX_ENABLED=1
export HUSKY_LMS1XX_PARENT='sensor_arch_mount_link'
export HUSKY_LMS1XX_XYZ='0 0 -0.01'
# export HUSKY_LASER_3D_ENABLED=1;
# export HUSKY_LASER_3D_PARENT='sensor_arch_mount_link'
# export HUSKY_LASER_3D_XYZ='0 0 -0.01'
roslaunch husky_gazebo husky_playpen.launch
