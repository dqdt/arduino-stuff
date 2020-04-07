#ifdef USB_SERIAL
#undef USB_SERIAL
#endif

// USB descriptors
#define LSB(n) ((n)&255)
#define MSB(n) (((n) >> 8) & 255)

#define ENDPOINT_TRANSMIT_ONLY 0x15
#define ENDPOINT_RECEIVE_ONLY 0x19

#define VENDOR_ID 0x16C0
#define PRODUCT_ID 0x0483
#define ENDPOINT0_SIZE 64
#define NUM_ENDPOINTS 4
#define NUM_USB_BUFFERS 14
#define NUM_INTERFACE 3
#define KEYBOARD_INTERFACE 0 // Keyboard
#define KEYBOARD_ENDPOINT 1
#define KEYBOARD_SIZE 8
#define KEYBOARD_INTERVAL 1

#define CONFIGURATION_DESC_SIZE 0
#define KEYBOARD_REPORT_DESC_SIZE 0

// Following `usb_desc.c`, and the example in: hid1_11.pdf, Appendix E
static uint8_t device_descriptor[] = {
    18,                               // bLength (1)
    0x01,                             // bDescriptorType (1)  Device Descriptor (0x01)
    0x10, 0x01,                       // bcdUSB (2)           USB 1.1 or 01.10
    0,                                // bDeviceClass (1)     If 0, each interface has its own class
    0,                                // bDeviceSubClass (1)
    0,                                // bDeviceProtocol (1)
    ENDPOINT0_SIZE,                   // bMaxPacketSize (1)   Valid values: {8, 16, 32, 64}
    LSB(VENDOR_ID), MSB(VENDOR_ID),   // idVendor (2)
    LSB(PRODUCT_ID), MSB(PRODUCT_ID), // idProduct (2)        Using USB_SERIAL codes
    0x73, 0x02,                       // bcdDevice (2)        Device Release Number. Copying Teensy. Arbitrary?
    1,                                // iManufacturer (1)    Use index 0 if there are no string descriptors
    2,                                // iProduct (1)
    3,                                // iSerialNumber (1)
    1,                                // bNumConfigurations (1)  One configuration
};

static uint8_t configuration_descriptor[] = {
    9,                            // bLength (1)
    0x02,                         // bDescriptorType (1)  Configuration Descriptor (0x02)
    LSB(CONFIGURATION_DESC_SIZE), // wTotalLength (2)    Total length of (configuration, interface, endpoint, HID)
    MSB(CONFIGURATION_DESC_SIZE), //                       descriptors for this configuration
    NUM_INTERFACE,                // bNumInterfaces (1)
    1,                            // bConfigurationValue (1)  Value used by SetConfiguration()
    0,                            // iConfiguration (1)  Index of String Descriptor (0 = none)
    0b10100000,                   // bmAttributes (1)    Power parameters. Remote Wakeup
    50,                           // bMaxPower (1)       Max Power Consumption in 2mA units. (50*2 = 100mA)
};

static uint8_t interface_descriptor_keyboard[] = {
    9,      // bLength (1)
    0x04,   // bDescriptorType (1)     Interface Descriptor (0x04)
    0,      // bInterfaceNumber (1)    Zero-based index, increment for each interface descriptor
    0,      // bAlternateSetting (1)   Used by SetInterface(). Alt settings for this interface.
    /**/ 1, // bNumEndpoints (1)       Endpoints used by this interface
    0x03,   // bInterfaceClass (1)     HID class code
    0x01,   // bInterfaceSubClass (1)  Supports boot protocol
    0x01,   // bInterfaceProtocol (1)  HID keyboard
    0,      // iInterface (1)          Index of String Descriptor (0 = none)
};

static uint8_t hid_descriptor_keyboard[] = {
    9,                              // bLength (1)
    0x21,                           // bDescriptorType (1)  HID Descriptor (HID 1.11 page 59)
    0x11, 0x01,                     // bcdHID (2)           HID 1.11
    0,                              // bCountryCode (1)     Not supported
    1,                              // bNumDescriptors (1)  Number of class descriptors (at least one Report Descriptor)
    0x22,                           // bDescriptorType (1)  Report Descriptor (HID 1.11 page 59)
    LSB(KEYBOARD_REPORT_DESC_SIZE), // wDescriptorLength (2)  Keyboard report size is 0x3F
    MSB(KEYBOARD_REPORT_DESC_SIZE),
    // [bDescriptorType]    Optional descriptors (e.g. phyiscal descriptor)
    // [wDescriptorLength]  3 bytes each
};

static uint8_t endpoint_descriptor_keyboard[] = {
    7,               // bLength (1)
    0x05,            // bDescriptorType (1)  Endpoint Descriptor (0x05)
    /**/ 0b10000001, // bEndpointAddress (1) Endpoint 1, IN
    0b11,            // bmAttributes (1)     Transfer Type: Interrupt Endpoint
    /**/ 8, 0,       // wMaxPacketSize (2)   Max packet size this endpoint can receive (8 bytes)
    /**/ 250 /*1*/,  // bInterval (1)        Polling interval (in milliseconds)
};

// Differs from HID 1.11, which had Logical Maximum(101) 0x65 for keys
// Teensy usb_desc.c says Logical Maximum(104) yet has 0x7F (which is 127)
//
// Check with HID1.11 page 38
static uint8_t report_descriptor_keyboard[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0xE0, //   Usage Minimum (224)
    0x29, 0xE7, //   Usage Maximum (231)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data, Variable, Absolute)   ; Modifier byte
    0x75, 0x08, //   Report Size (8)
    0x95, 0x01, //   Report Count (1)
    0x81, 0x01, //   Input (Constant)                   ; Reserved byte
    0x05, 0x08, //   Usage Page (Page# for LEDs)
    0x19, 0x01, //   Usage Minimum (1)
    0x29, 0x05, //   Usage Maximum (5)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x05, //   Report Count (5)
    0x91, 0x02, //   Output (Data, Variable, Absolute)  ; LED report
    0x75, 0x03, //   Report Size (3)
    0x95, 0x01, //   Report Count (1)
    0x91, 0x01, //   Output (Constant)                  ; LED report padding
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0x00, //   Usage Minimum (0)
    0x29, 0x7F, //   Usage Maximum (127)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x7F, //   Logical Maximum (127)D
    0x75, 0x08, //   Report Size (8)
    0x95, 0x06, //   Report Count (6)
    0x81, 0x00, //   Input (Data, Array)                ; Key arrays (6 bytes)
    0xC0,       // End Collection
};

typedef struct
{
    uint16_t wValue;
    uint16_t wIndex;
    const uint8_t *addr;
    uint16_t length;
} usb_descriptor_list_t;

// An array of ENDPOINTx_CONFIG which is ENDPOINT_TRANSMIT_ONLY, etc. or ENDPOINT_UNUSED
const uint8_t usb_endpoint_config_table[NUM_ENDPOINTS];

// An array of {wValue, wIndex, addressof(descriptor_array), sizeof(descriptor_array)}
const usb_descriptor_list_t usb_descriptor_list[];

#include <Arduino.h>

int main()
{
    // Serial.begin(115200);
    Serial2.begin(38400);

    // Keyboard.print('a');

    while (1)
    {
        // Serial.println("hello com4");
        Serial2.println("hello from Teensy2");
        delay(1000);
    }
}