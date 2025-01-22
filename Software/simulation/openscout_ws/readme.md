This is a ROS2 package for virtually simulating the OpenScout robot.

## Environment
[Ubuntu Noble 24.04.1](https://releases.ubuntu.com/noble/) 

[ROS2 Jazzy](https://docs.ros.org/en/jazzy/Installation.html)

[Gazebo Harmonic](https://gazebosim.org/docs/harmonic/ros_installation/)
 
### Further info
During the time of writing (2025), Gazebo Harmonic does not tolerate running in a virtual machine very well. It is recommened to run it on a computer with Ubuntu Noble installed natively. 

Additionally, when installing Gazebo Harmonic ensure that you do not install the standalone version. Instead first install ROS2 Jazzy, and then opt for [the version of Gazebo integrated into ROS2 Jazzy.](https://docs.ros.org/en/jazzy/Installation.html)

## Guide to running the package (On the default Ubuntu system)

First make sure you have teleop twist keyboard installed.
````
sudo apt-get install ros-jazzy-teleop-twist-keyboard
````

Download or clone the repository openscout_ws, and put it in the **home** folder.

Open the Terminal and enter the following commands.

````
source /opt/ros/jazzy/setup.bash
cd ~/openscout_ws
colcon build
source install/setup.bash
ros2 launch openscout openscout.launch.py
````

This should open the simulation.

From there you can try moving the openscout with teleop keyboard.
In a new terminal enter the following:
````
source /opt/ros/jazzy/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
````

## Once in the simulation

Try driving over the slope with one wheel. This showcases the joint in the middle of the robot.

Feel free to make edits to the evironment or robot to test whatever you like.

Its possible to make a python file to control the openscout, using ROS2 topics.