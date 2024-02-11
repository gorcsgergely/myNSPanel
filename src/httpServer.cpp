#include <WebServer.h>
#include "httpServer.h"
#include "web.h"
#include <ArduinoJson.h>

httpServer::httpServer():_server(80),_nextion(115200){

}

WebServer* httpServer::getServer(){
  return &_server;
}

void httpServer::handleClient(){
  _server.handleClient();
}

void httpServer::begin(){
  _server.begin();
}

void httpServer::handleRootPath() {            //Handler for the rooth path
   
  String html = MAIN_page;
  _server.send(200, "text/html", html); 
}

void httpServer::handleTftUploadPath() {  
   
  String html = TFTUPLOAD_page;
  _server.send(200, "text/html", html); 
}

void httpServer::handleSuccessPath() {
   
  String html = SUCCESS_page;
  _server.send(200, "text/html", html);  
}

void httpServer::handleFailurePath() {
   
  String html = FAILURE_page;
  _server.send(200, "text/html", html); 
}

void httpServer::handleConfigurePath() {
   
  String html = CONFIGURE_page;
  _server.send(200, "text/html", html); 
}

// handle the file uploads
boolean httpServer::handleFileUpload(){
  HTTPUpload& upload = _server.upload();
  
  // Check if file seems valid nextion tft file
  if(!upload.filename.endsWith(F(".tft"))){
    _server.send(500, F("text/plain"), F("ONLY TFT FILES ALLOWED\n"));
    return false;
  }
  
  if(!_result){
    // Redirect the client to the failure page
    _server.sendHeader(F("Location"),"/failure?reason=" + _nextion.statusMessage);
    _server.send(303);
    return false;
  }

  if(upload.status == UPLOAD_FILE_START){
    #ifdef DEBUG
    Serial.println(F("\nFile received. Update Nextion..."));
    #endif

    // Prepare the Nextion display by seting up serial and telling it the file size to expect
    _result = _nextion.prepareUpload(_fileSize);
    
    if(_result){
      #ifdef DEBUG
      Serial.print(F("Start upload. File size is: "));
      Serial.print(_fileSize);
      Serial.println(F(" bytes"));
      #endif
    }else{
      #ifdef DEBUG
      Serial.println(_nextion.statusMessage + "\n");
      #endif
      return false;
    }
    
  }else if(upload.status == UPLOAD_FILE_WRITE){
    // Write the received bytes to the nextion
    _result = _nextion.upload(upload.buf, upload.currentSize);
    
    if(_result){
      #ifdef DEBUG
      Serial.print(F("."));
      #endif
    }else{
      #ifdef DEBUG
      Serial.println(_nextion.statusMessage + "\n");
      #endif
      return false;
    }
  
  }else if(upload.status == UPLOAD_FILE_END){

    // End the serial connection to the Nextion and softrest it
    _nextion.end();
    Serial.println("");
    return true;
  }
  return true;
}

void httpServer::setup(){

  _server.on("/upload", HTTP_POST, [&](){ 
    #ifdef DEBUG
    Serial.println(F("Succesfully updated Nextion!\n"));
    #endif
    // Redirect the client to the success page after handeling the file upload
    _server.sendHeader(F("Location"),F("/success"));
    _server.send(303);
    return true;
    },
    // Receive and save the file
    std::bind(&httpServer::handleFileUpload, this)
  );

  _server.on("/", std::bind(&httpServer::handleRootPath, this));    //Associate the handler function to the path
  _server.on("/upload", std::bind(&httpServer::handleTftUploadPath, this));
  _server.on("/success", std::bind(&httpServer::handleSuccessPath, this));
  _server.on("/failure", std::bind(&httpServer::handleFailurePath, this));
  _server.on("/configure",std::bind(&httpServer::handleConfigurePath, this));

    // receive fileSize once a file is selected (Workaround as the file content-length is of by +/- 200 bytes. Known issue: https://github.com/esp8266/Arduino/issues/3787)
  _server.on("/fs", HTTP_POST, [&](){
    _fileSize = _server.arg(F("fileSize")).toInt();
    _server.send(200, F("text/plain"), "");
  });

}