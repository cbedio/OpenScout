# Open AgriScout

## An Open Source Hardware Agricultural Mobile Robot
Open AgriScout is a low-cost open source hardware and software mobile robot that can be used for both indoor and outdoor tasks, transporting up to 15kg of payload. The robot is designed to be easily and cheaply (200USD) buildable and modifiable by non-specialists, and to function as a new standard physical platform for robotics research and real-world tasks, replacing current proprietary options. Example applications include last mile and factory floor delivery, site survey and monitoring, and agricultural crop operations.

OpenAgriscout is made of standard sizes aluminium extrusions making it very modular. That means its chassis design depends on the use case and the needs of the user. The robot provides the bare minimum(wizard-of-oz tele-operation and software for controlled differential drive) and future versions will introduce more chassis configurations, ROS/ROS2 integration and gazebo urdf files.

<p float="left">
  <img src="Documentation/Images/agriscout_on_concrete.png" title="Robot with hinge in the middle configuration" width="400" height="300"/>
  <img src="Documentation/Images/agriscout_incline.png" title="Robot with hinge in the middle configuration" width="400" height="300"/> 
</p>

## Demonstration

https://user-images.githubusercontent.com/44243266/164105180-839fc627-df63-4446-af7d-90f431a007b9.mp4

## Getting started
Materials used:
The robot consists of 200mm & 300mm 20x20 aluminium extrusions connected with 90 degree angle joints so the width, length and its height can be highly adjustable. We suggest also the [90:1 12V CQrobot](https://www.amazon.co.uk/CQRobot-90-Gearmotor-oz-Diameter/dp/B0887RR8SH) motor with encoder, as 4 of them provide enough traction to carry big payloads. Finally, an Arduino Mega is necessary as it provides enough interrupt pins for the RF receiver and the motor encoders.

The full bill of materials depends on each configuration and for more details please refer to the tutorials.

### Assembly Tutorial:

## * [Open AgriScout with Lazy Susan bearing](Hardware/robot_with_lazy_susan_bearing/README.md)


## How to contribute
While we try to keep this project open source you are free to make your own choice of materials and adapt the robot to your needs. However, we kindly request you to stick to the suggested 200mm & 300mm 20x20 aluminum extrusions, to allow other users disassemble their current configuration and try out yours! If you use OpenAgriscout for your project, please open a PR with your configuration and it will added in the tutorials. 

The general process of contributing on GitHub is widely documented however the outline process is below:

1. Identify where you want to host the project locally. This could be a Music Makers projects folder for example. 


1. Clone or fork the repository using GitHub desktop or the CLI into this location (CLI is recommended as this helps you become more familiar with Git in general). You can do this with the following command:

    ```bash
    git clone https://github.com/put_our_project_here
    ```

1. Update the project and then make a pull request!

## License

Add licence here
