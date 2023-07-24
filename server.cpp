#include <queue>
#include <memory>
#include <thread>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

//MSG IDs
#define CONTACT_CARD_ID       0x01
#define CONTACTLESS_CARD_ID   0x02
#define MIFARE_CARD_ID        0x03
#define SERVICE_OPERATIONS_ID 0x04
#define NXP_NTAG_CARD_ID      0x05
#define GUI_OPERATIONS_ID     0x06

//Command types
#define COMMAND_MSG 0x01
#define SUCCESS_MSG 0x02
#define FAILURE_MSG 0x03
#define PENDNG_MSG  0x04

class SharedCounter {
    public:
        uint8_t count;
        SharedCounter() {
            this.count = 0;
        }
}

class ToDevice {
    public:
        SharedCounter shared_counter;
        std::queue<std::shared_ptr<char>> out_queue;

        ToDevice(SharedCounter shared_counter) {
            this.shared_counter = shared_counter;
        }

        void to_device_data_processing() {
            while (!this.out_queue.empty()) {
                this.shared_counter++;
                packet = out_queue.pop();
                packet = replace_msgid(packet, this.shared_counter);
                raw_send(packet); //low-level send function
            }
        }
            
        void send_data_to_device(std::shared_ptr<char> data) {
            out_queue.push(data);
        }
}

class FromDevice:
    public:
        SharedCounter shared_counter;

        FromDevice(SharedCounter shared_counter) {
            this.shared_counter = shared_counter;
            std::thread(this.get_data_from_device);
            std::thread(this.to_device_data_processing);
            std::thread(this.from_device_data_processing);
        }

        void from_device_data_processing() {
            while (!in_queue.empty()) {
                packet_from_device = in_queue.pop();
                msgid_from_device = parce_msgid(packet_from_device);
                if (msgid_from_device == this.shared_counter) {
                    module_id = parce_module_id(packet_from_device)
                    if (module_id == CONTACT_CARD_ID) {
                        contact_card_callback(packet_from_device);
                    } else if (module_id == CONTACTLESS_CARD_ID) {
                        contactless_card_callback(packet_from_device);
                    } else if (module_id == MIFARE_CARD_ID) {
                        mifare_card_callback(packet_from_device);
                    } else if (module_id == SERVICE_OPERATIONS_ID) {
                        service_operations_callback(packet_from_device);
                    } else if (module_id == NXP_NTAG_CARD_ID) {
                        nxp_ntag_card_callback(packet_from_device);
                    } else if (module_id == GUI_OPERATIONS_ID) {
                        gui_operations_callback(packet_from_device);
                    }
                }
            }
        }

        void get_data_from_device() {
            in_queue.push(raw_receive()); //#low-level receive function
        }

class Module {
    public:
        Module(uint8_t module_type, void* send_func) {
            this.last_packet = nullptr;
            this.send = send_func;
            this.module_type = module_type;
        }

        void callback(std::shared_ptr<char> data) {
            this.last_packet = data;
        }

        std::pair <int, std::shared_ptr<char> > send_data(std::shared_ptr<char> data, int command_type) {
            this.last_packet = nullptr;

            std::string bin_cmd = string_to_base64(data);
            std::string bin_packet = ("AC" + 
                         bin_cmd.length() +
                         msg_id +
                         self.module_type +
                         type_to_bin(command_type) +
                         bin_cmd);    
            std::shared_ptr<char> packet = to_pointer(bin_packet + calc_chechsum(bin_packet)); //rewrite normally

            send(packet);
            try {
                while (!(this.last_packet == nullptr));
                
                if (check_checksum(packet) == false) {
                    throw("checksum error\n");
                }
                int type_msg = parce_type(self.last_packet)
                std::shared_ptr<char> payload = parce_payload(self.last_packet)

                return std::make_pair(type_msg, payload);

            }
            catch(std::string exception) {
                cout << exception;
            }

        }
}

int main() {

    SharedCounter* shared_counter = new SharedCounter;

    ToDevice to_device = ToDevice(shared_counter);

    Module contact_card_module = Module(CONTACT_CARD_ID, &to_device.send_data);
    Module contactless_card_module = Module(CONTACTLESS_CARD_ID, &to_device.send_data);
    Module mifare_card_module = Module(MIFARE_CARD_ID, &to_device.send_data);
    Module service_operations = Module(SERVICE_OPERATIONS_ID, &to_device.send_data);
    Module nxp_ntag_card_module = Module(NXP_NTAG_CARD_ID, &to_device.send_data);
    Module gui_operations = Module(GUI_OPERATIONS_ID, &to_device.send_data);

    FromDevice from_device = FromDevice(shared_counter,
                            contact_card_module.callback, 
                            contactless_card_module.callback, 
                            mifare_card_module.callback, 
                            service_operations.callback, 
                            nxp_ntag_card_module.callback, 
                            gui_operations.callback);


    //send and receive data
    std::pair <int, std::shared_ptr<char> > type_msg_and_payload = service_operations.send_data('{”Leds”:{”blue”:1”red”:0”green”:0}}', 0x01);
    if type_msg_and_payload.First == 0x02: print("Success");
    if type_msg_and_payload.First == 0x03: print("Failure");
    return 0;
}