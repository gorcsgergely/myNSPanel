#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WebServer.h>
#include "config.h"
#include "ESPNexUpload.h"

class httpServer{
    public:
        httpServer();
        WebServer* getServer();
        void handleClient();
        void begin();
        void handleRootPath();
        void handleConfigurePath();
        void handleFailurePath();
        void handleSuccessPath();
        void handleTftUploadPath();
        bool handleFileUpload();
        void setup();

    private:
        WebServer _server;
        // init Nextion object
    private:
        ESPNexUpload _nextion;
        // used only internally
        int _fileSize  = 0;
        bool _result   = true;
};

#endif