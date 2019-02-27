# Button Box

This project is not really "button box", but it's the working title for the project until I can come up with a better name.

# Useful terminal commands

This will build the platform and provision the device w/ wifi setup as well as IoT certs.

`mos build --platform=esp32 && mos flash && mos wifi "hello friends" password123 && mos aws-iot-setup --aws-region us-west-2`
