import math
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from ackermann_msgs.msg import AckermannDriveStamped

class AckermannConverter(Node):
    def __init__(self):
        super().__init__('ackermann_converter')
        self.declare_parameter('wheelbase', 0.3302)
        self.declare_parameter('max_steering', 0.4189)
        self.wheelbase = self.get_parameter('wheelbase').value
        self.max_steering = self.get_parameter('max_steering').value
        
        self.sub = self.create_subscription(Twist, 'cmd_vel_nav', self.callback, 10)
        self.pub = self.create_publisher(AckermannDriveStamped, '/drive', 10)


    def callback(self, twist):
        v = twist.linear.x
        omega = twist.angular.z

        if abs(v) < 1e-3:
            steering = 0.0
        else:
            steering = math.atan(self.wheelbase * omega / v)

        steering = max(-self.max_steering, min(self.max_steering, steering))

        msg = AckermannDriveStamped()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.drive.speed = v
        msg.drive.steering_angle = steering
        self.pub.publish(msg)

    
def main():
    rclpy.init()
    rclpy.spin(AckermannConverter())
    rclpy.shutdown()
