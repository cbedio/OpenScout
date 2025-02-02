This is a ROS2 package for virtually simulating the OpenScout robot.

## Environment
[Ubuntu Noble 24.04.1](https://releases.ubuntu.com/noble/) 

[ROS2 Jazzy](https://docs.ros.org/en/jazzy/Installation.html)

[Gazebo Harmonic](https://gazebosim.org/docs/harmonic/ros_installation/)
 
### Further info
During the time of writing (2025), Gazebo Harmonic does not tolerate running in a virtual machine very well. It is recommended to run it on a computer with Ubuntu Noble installed natively. 

Additionally, when installing Gazebo Harmonic ensure that you do not install the standalone version. Instead first install ROS2 Jazzy, and then opt for [the version of Gazebo integrated into ROS2 Jazzy.](https://docs.ros.org/en/jazzy/Installation.html)

![alt text](https://github.com/ilovemicroplastics/OpenScout/blob/main/Software/simulation/openscout_ws/openscout_sim.png)

## Guide to running the package (On the default Ubuntu system)

First prepare the environment, as listed above.

Then make sure you have teleop twist keyboard installed.
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

Try driving over the slope with one side. This showcases the joint in the middle of the robot.

Feel free to make edits to the SDF to test whatever you like.

Its possible to make a python file to control the openscout using ROS2 topics.

## Example video of simulation running once set up

https://github.com/user-attachments/assets/db36529f-7577-4ad0-a52f-730f6c3506c2
