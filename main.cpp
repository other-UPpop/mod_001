#include <dpp/dpp.h>
#include <iostream>
#include <map>
#include <chrono>
#include <string>
#include <sstream>

const std::string BOT_TOKEN = "YOUR_SECURE_TOKEN_HERE_AFTER_RESET"; 

std::map<dpp::snowflake, std::chrono::time_point<std::chrono::system_clock>> voice_join_times;

std::string format_duration(long long total_seconds) {
    long long hours = total_seconds / 3600;
    long long minutes = (total_seconds % 3600) / 60;
    long long seconds = total_seconds % 60;
    
    std::stringstream ss;
    if (hours > 0) ss << hours << "時間 ";
    if (minutes > 0) ss << minutes << "分 ";
    ss << seconds << "秒";
    
    return ss.str();
}

int main() {
    dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content | dpp::i_voice_states);
    bot.on_log(dpp::utility::cout_logger());
    
    bot.on_message_create([&bot](const dpp::message_create_t& event){
        if(event.msg.author.is_bot()) 
            return;
        
        bot.message_add_reaction(event.msg.id, event.msg.channel_id, "👍");
    });

    bot.on_voice_state_update([&bot](const dpp::voice_state_update_t& event) {
        dpp::snowflake user_id = event.state.user_id;
        dpp::snowflake channel_id = event.state.channel_id;

        if (channel_id != 0) {
            if (voice_join_times.count(user_id) == 0) {
                 voice_join_times[user_id] = std::chrono::system_clock::now();
            }
        }
        else {
            auto it = voice_join_times.find(user_id);
            
            if (it != voice_join_times.end()) {
                auto join_time = it->second;
                auto leave_time = std::chrono::system_clock::now();
                
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(leave_time - join_time);
                long long total_seconds = duration.count();
                std::string duration_str = format_duration(total_seconds);
                
                voice_join_times.erase(it);
                
                dpp::guild *g = dpp::find_guild(event.state.guild_id);
                if (g) {
                    dpp::channel *c = dpp::find_channel(g->system_channel_id);
                    if (c) {
                        std::string message_content = "<@" + std::to_string(user_id) + ">さんの通話時間は " + duration_str + " でした。";
                        bot.message_create(dpp::message(c->id, message_content));
                    }
                }
            }
        }
    });
    
    bot.start(dpp::st_wait);
    return 0;
}