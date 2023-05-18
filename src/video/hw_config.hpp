#pragma once
#include <cassert>
#include <string>
#include <memory>

#include "video/hw_config_base.hpp"
#include "video/hw_config_u30.hpp"
#include "video/hw_config_v70.hpp"

struct hw_config
{
    static std::shared_ptr<hw_config_base> get_instance()
    {
        static std::shared_ptr<hw_config_base> instance;

        if (!instance)
        {
            std::string card = "v70";
            if (const char* e = std::getenv("CARD_VIDEO")) card = e;

            if (card == "u30")
                instance = std::make_shared<hw_config_u30>();
            else
                instance = std::make_shared<hw_config_v70>();
        }

        return instance;
    }
};
