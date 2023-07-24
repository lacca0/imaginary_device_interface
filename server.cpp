#include <queue>
#include <memory>
#include <thread>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum MODULE_ID {
    CONTACT_CARD_ID,
    CONTACTLESS_CARD_ID,
    MIFARE_CARD_ID,
    SERVICE_OPERATIONS_ID,
    NXP_NTAG_CARD_ID,
    GUI_OPERATIONS_ID
};

enum COMMAND_TYPE {
    COMMAND_MSG,
    SUCCESS_MSG,
    FAILURE_MSG,
    PENDNG_MSG
};

class ToDevice {
    public:
        std::unique_ptr<uint8_t> shared_counter;
        std::queue<std::shared_ptr<char> > out_queue;

        ToDevice(unique_ptr<uint8_t> shared_counter) {
            this->shared_counter = shared_counter;
            std::thread(this->to_device_data_processing);
            std::thread(this->send_data_to_device);
        }

        void to_device_data_processing() {
            while (!this->out_queue.empty()) {
                (this->shared_counter)* ++;
                packet = this->out_queue.pop();
                packet = replace_msgid(packet, this->shared_counter);
                raw_send(packet); //low-level send function
            }
        }
            
        void send_data_to_device(std::shared_ptr<char> data) {
            this->out_queue.push(data);
        }
};

class FromDevice {
    public:
        std::queue<std::shared_ptr<char> > in_queue;
        void* contact_card_callback;
        void* contactless_card_callback;
        void* mifare_card_callback;
        void* service_operations_callback;
        void* nxp_ntag_card_callback;
        void* gui_operations_callback;

        FromDevice(void* contact_card_callback,
                    void* contactless_card_callback,
                    void* mifare_card_callback,
                    void* service_operations_callback,
                    void* nxp_ntag_card_callback,
                    void* gui_operations_callback) {
            this->shared_counter = shared_counter;
            this->contact_card_callback = contact_card_callback;
            this->contactless_card_callback = contactless_card_callback;
            this->mifare_card_callback = mifare_card_callback;
            this->service_operations_callback = service_operations_callback;
            this->nxp_ntag_card_callback = nxp_ntag_card_callback;
            this->gui_operations_callback = gui_operations_callback;
            std::thread(this->get_data_from_device);
            std::thread(this->from_device_data_processing); 
        }

        void from_device_data_processing() {
            while (!in_queue.empty()) {
                packet_from_device = in_queue.pop();
                MODULE_ID module_id = parce_module_id(packet_from_device)
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

        void get_data_from_device() {
            this->in_queue.push(raw_receive()); //#low-level receive function
        }
};

class Module {
    private:
        uint8_t last_MSGID;
    public:
        std::unique_ptr<uint8_t> shared_counter;

        Module(std::unique_ptr<uint8_t> shared_counter, uint8_t module_type, void* send_func) {
            this->shared_counter = shared_counter;
            this->last_packet = nullptr;
            this->send = send_func;
            this->module_type = module_type;
        }

        void callback(std::shared_ptr<char> data) {
            this->last_packet = data;
        }

        std::pair <int, std::shared_ptr<char> > send_data(std::shared_ptr<char> data, COMMAND_TYPE command_type) {
            this->last_packet = nullptr;
            this->last_MSGID = msg_id;
            std::string bin_cmd = string_to_base64(data);
            std::string bin_packet = (std::str("AC") + 
                                    (bin_cmd + msg_id + this->module_type + type_to_bin(command_type)).length() +
                                    msg_id +
                                    this->module_type +
                                    type_to_bin(command_type) +
                                    bin_cmd);    
            std::shared_ptr<char> packet = to_pointer(bin_packet + calc_chechsum(bin_packet)); //rewrite normally

            send(packet);
            try {
                while (!(this->last_packet == nullptr));
                if (check_msgid(packet) != last_MSGID) {
                    throw("out of order\n");
                }
                if (check_checksum(packet) == false) {
                    throw("checksum error\n");
                }
                int type_msg = parce_type(self.last_packet);
                std::shared_ptr<char> payload = parce_payload(self.last_packet);

                return std::make_pair(type_msg, payload);

            }
            catch(std::string exception) {
                cout << exception;
            }

        }
};

int main() {

    std::unique_ptr<uint8_t> shared_counter = std::make_unique<uint8_t>(0);

    ToDevice to_device = ToDevice(shared_counter);

    Module contact_card_module = Module(shared_counter, CONTACT_CARD_ID, &to_device.send_data);
    Module contactless_card_module = Module(shared_counter, CONTACTLESS_CARD_ID, &to_device.send_data);
    Module mifare_card_module = Module(shared_counter, MIFARE_CARD_ID, &to_device.send_data);
    Module service_operations = Module(shared_counter, SERVICE_OPERATIONS_ID, &to_device.send_data);
    Module nxp_ntag_card_module = Module(shared_counter, NXP_NTAG_CARD_ID, &to_device.send_data);
    Module gui_operations = Module(shared_counter, GUI_OPERATIONS_ID, &to_device.send_data);

    FromDevice from_device = FromDevice(contact_card_module.callback, 
                                        contactless_card_module.callback, 
                                        mifare_card_module.callback, 
                                        service_operations.callback, 
                                        nxp_ntag_card_module.callback, 
                                        gui_operations.callback);


    //send and receive data
    std::pair <int, std::shared_ptr<char> > type_msg_and_payload = service_operations.send_data('{”Leds”:{”blue”:1”red”:0”green”:0}}', 0x01);
    if (type_msg_and_payload.First == 0x02) {
        cout << "Success";
    } else if (type_msg_and_payload.First == 0x03) { 
        cout << "Failure";
    }
    return 0;
}