import sys
import logging
from jadepy import JadeAPI

LOGGING = True
NETWORK = 'testnet'

# We can test with the gdk http_request() function if we have the wheel installed
# The default is to use the simple built-in http requests client.
USE_GDK_HTTP_CLIENT = False

# script to test user auth and pinserver interaction
# need to run the pinserver as in docker as described in its readme
# (or directly with flask run)

# Enable jade logging
if LOGGING:
    jadehandler = logging.StreamHandler()
    jadehandler.setLevel(logging.INFO)

    logger = logging.getLogger('jade')
    logger.setLevel(logging.DEBUG)
    logger.addHandler(jadehandler)

    logger = logging.getLogger('jade-device')
    logger.setLevel(logging.DEBUG)
    logger.addHandler(jadehandler)


# We can test with the gdk http_request() function if we have the wheel installed
http_request_fn = None
if USE_GDK_HTTP_CLIENT:
    import json
    import greenaddress as gdk

    gdk.init({})
    gdk_session = gdk.Session({'name': 'mainnet'})

    def http_request_fn(params):
        reply = gdk.http_request(gdk_session.session_obj, json.dumps(params))
        return json.loads(reply)


if len(sys.argv) > 1 and sys.argv[1] == 'ble':
    print('Fetching jade version info over BLE')
    serial_number = sys.argv[2] if len(sys.argv) > 2 else None
    create_jade_fn = JadeAPI.create_ble
    kwargs = {'serial_number': serial_number}
else:
    print('Fetching jade version info over serial')
    serial_device = sys.argv[2] if len(sys.argv) > 2 else None
    create_jade_fn = JadeAPI.create_serial
    kwargs = {'device': serial_device, 'timeout': 120}

print("Connecting...")
with create_jade_fn(**kwargs) as jade:
    print("Connected: {}".format(jade.get_version_info()))

    # Tell Jade to auth the user on the hw
    # Note: this requires a pinserver to be running
    while jade.auth_user(NETWORK, http_request_fn) is not True:
        print("Error - please try again")

    # Just a couple of test calls that mimic what gdk-logon does
    print(jade.get_xpub(NETWORK, []))
    print(jade.sign_message([1195487518], "greenaddress.it      login ABCDE"))
