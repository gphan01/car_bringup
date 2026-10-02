"""Full sim autonomy stack in one command.

    ros2 launch /sim_ws/src/car_bringup/launch/sim_bringup_launch.py

Order: F1TENTH sim (+ RViz) -> slam_toolbox -> converter -> Nav2 (delayed,
so map -> odom exists before Nav2's costmaps configure).
Ctrl+C here stops everything.

All nodes on wall time: the F1TENTH bridge publishes no /clock.
"""
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

PKG_SRC = '/sim_ws/src/car_bringup'
CONFIG = os.path.join(PKG_SRC, 'config')
LAUNCH = os.path.join(PKG_SRC, 'launch')


def generate_launch_description():
    sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory('f1tenth_gym_ros'),
            'launch', 'gym_bridge_launch.py')))

    slam = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory('slam_toolbox'),
            'launch', 'online_async_launch.py')),
        launch_arguments={
            'params_file': os.path.join(CONFIG, 'mapper_params_online_async.yaml'),
            'use_sim_time': 'false',
        }.items())

    converter = Node(
        package='car_bringup',
        executable='ackermann_converter',
        output='screen')

    nav2 = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(LAUNCH, 'nav2_launch.py')),
        launch_arguments={
            'params_file': os.path.join(CONFIG, 'nav2_params.yaml'),
            'use_sim_time': 'false',
        }.items())

    return LaunchDescription([
        sim,
        slam,
        converter,
        TimerAction(period=8.0, actions=[nav2]),
    ])
