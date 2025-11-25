#include <dpp/dpp.h>
#include <iostream>
#include <map>
#include <chrono>

std::map<dpp::snowflake, std::chrono::time_point<std::chrono::system_clock>> voice_join_times;

int main() {
    dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content | dpp::i_voice_states);
