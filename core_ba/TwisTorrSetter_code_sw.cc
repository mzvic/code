#include <iostream>
#include <mutex>
#include <string>
#include "client.h"
#include "core.grpc.pb.h"

using namespace core;
using google::protobuf::Timestamp;
using std::string;

Bundle* publishing_bundle;
PublisherClient* publisher_client;

bool exit_flag = false;
std::mutex signal_mutex;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <value> <variable>\n";
    return 1;
  }
  //std::cout << "Arguments: " << argv[1] << ", " << argv[2] << ", " << argv[3] << "\n";

  int variable;
  float value;
  try {
    value = std::stoi(argv[1]);
    variable = std::stoi(argv[2]);
  } catch (const std::invalid_argument& e) {
    std::cerr << "Invalid argument: " << e.what() << "\n";
    return 1;
  } catch (const std::out_of_range& e) {
    std::cerr << "Out of range: " << e.what() << "\n";
    return 1;
  }

  Timestamp timestamp;
  publisher_client = new PublisherClient();
  publishing_bundle = new Bundle();
  publishing_bundle->set_type(DATA_TT_SET);

  publishing_bundle->clear_value();
  publishing_bundle->add_value(value);
  publishing_bundle->add_value(variable);
  publisher_client->Publish(*publishing_bundle, timestamp);

  delete publisher_client;
  delete publishing_bundle;

  return 0;
}

