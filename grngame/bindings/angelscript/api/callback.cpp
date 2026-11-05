
#include "callback.h"

#include "grngame/dev/logging.h"

void MessageCallback(const asSMessageInfo *msg, void *param)
{
    if (msg->type == asMSGTYPE_ERROR)
        LOG_ERROR("%s", msg->message);
    else if (msg->type == asMSGTYPE_WARNING)
        LOG_WARNING("%s", msg->message);
    else
    {
        LOG_INFO("%s", msg->message);
    }
}
