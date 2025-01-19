import os
from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    # package name
    name = "openscout"

    # world file launch argument
    world_arg = DeclareLaunchArgument(
        'world',
        default_value=os.path.join(
        get_package_share_directory(name),
        'model',
        'openscout.sdf'),
        description='choose world file for gazebo'
    )

    # read bridge config
    world_file = LaunchConfiguration('world')

    gz_bridge_params_path = os.path.join(
        get_package_share_directory(name),
        'config',
        'gz_bridge.yaml'
    )

    # locate gazebo launch file
    gazebo_pkg_launch = PythonLaunchDescriptionSource(
        os.path.join(
            get_package_share_directory('ros_gz_sim'),
            'launch',
            'gz_sim.launch.py'
        )
    )

    # add arguments to gazebo launch file
    gazebo_launch = IncludeLaunchDescription(
        gazebo_pkg_launch,
        launch_arguments={
            'gz_args': [f'-r -v 4 ', world_file],
            'on_exit_shutdown': 'true'
        }.items()
    )

    # add node for ros-gazebo bridge
    gz_bridge_node = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '--ros-args', '-p',
            f'config_file:={gz_bridge_params_path}'
        ],
        output='screen'
    )

    return LaunchDescription([
        world_arg,
        gazebo_launch,
        gz_bridge_node,
    ])