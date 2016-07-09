var buffer_size = 1300;
// Changes with sampling rate
// 10 Hz : ms = 100 | 25 Hz : ms = 40 | 50 Hz : ms = 20 | 100 Hz : ms = 10
//var ms = 40; 
var time_samples = 2;
var xarray = [];
var yarray = [];
var zarray = [];
var timearray = [];
var activityType = [];
var typeStart = [];
var typeEnd = [];
var sendData = {};
var username = localStorage.getItem("username");
//var email = "kostakiskakos@hotmail.com";
var email = localStorage.getItem("email");
var numberOfMessages = 0;
var request = new XMLHttpRequest();
var emailReq = new XMLHttpRequest();
var possibleFall = false;
var peakIndex = 0;
var overlappingVectorSumLeft = [];
var overlappingVectorSumRight = [];
var numberOfFalls = 0;
var fallDetected = false;
var location = "";
//var emailData = "";
var lat = 0.0;
var long = 0.0;

console.log("* * *Mobile phone says hello!!!* * *");
console.log(username);
console.log(email);

function convertBytesToAccel(msB,lsB){
  var temp = msB | 127;
  if (temp!=255)
    return (msB*100 + lsB);
  else{
    msB = msB & 127;
    return -(msB*100 + lsB);
  }  
}

function convertBytesToTime(byte6,byte5,byte4,byte3,byte2,byte1,byte0){
  return (byte6*1000000000000 + byte5*10000000000 + byte4*100000000 + byte3*1000000 + byte2*10000 + byte1*100 + byte0);
}

// Function to send a message to the Pebble using AppMessage API
// We are currently only sending a message using the "status" appKey defined in appinfo.json/Settings
function sendMessage() {
	Pebble.sendAppMessage({"status": 0}, messageSuccessHandler, messageFailureHandler);
}

// Called when the message send attempt succeeds
function messageSuccessHandler() {
  console.log("Message send succeeded.");  
}

// Called when the message send attempt fails
function messageFailureHandler() {
  console.log("Message send failed.");
  sendMessage();
}

function geolocationSuccess(position){
  lat = position.coords.latitude;
  long = position.coords.longitude;
  console.log("Latitude: "+ lat + " longitude: " + long);
  location = "Latitude: "+ lat + " longitude: " + long;
}

function geolocationError(error) {
  console.log("code: "    + error.code    + "\n" + "message:" + error.message + "\n");
}


Pebble.addEventListener('showConfiguration', function() {
  //var url = 'https://e088a03dc67dacfb01333eea79f5192d2b79960c.googledrive.com/host/0By0ivjD6euU1ZDlBLWZ4YmFMT2s/form.html';
  var url = 'http://83.212.115.163/config.html';
  console.log('Showing configuration page: '+ url);
  Pebble.openURL(url);
});


Pebble.addEventListener('webviewclosed', function(e) {
  // Decode the user's preferences
  var configData = JSON.parse(decodeURIComponent(e.response));
  username = configData.userName;
  localStorage.setItem("username",username);
  console.log("username set: "+username);
  
  email = configData.email;
  localStorage.setItem("email",email);
  console.log("email set: "+email);
});



// Called when JS is ready
Pebble.addEventListener("ready", function(e) {
  console.log("JS is ready!");
  sendMessage();
});
						


// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
  
    var vectorSum = [];
    var variance = 0;
    var mean = 0;
    var meanCos = 0;
    var meanDistance = 0;
    var tempMinimum = 0;
    var fallDistance = 0;  
  
    navigator.geolocation.getCurrentPosition(geolocationSuccess,[geolocationError]);
  
    if(e.payload.fallKey !== undefined){
      //send email
      console.log("JAVASCRIPT HERE: sending email");
      //navigator.geolocation.getCurrentPosition(geolocationSuccess,[geolocationError]);
      //emailData = username+" needs help ASAP. His location is: "+location;
      //console.log(emailData);
      
      emailReq.open('POST','http://83.212.115.163/EmailSender.php?username='+username+'&latitude='+lat+'&longitude='+long+'&email='+email,true);
      emailReq.setRequestHeader('Content-Type', 'application/json; charset=utf-8');   
      emailReq.send(/*JSON.stringify(emailData)*/);
      
    }
    
    if(e.payload.timevalue !== undefined){
      
      numberOfMessages++;
      for(var i=0;i<buffer_size;i++){
       
        xarray.push(convertBytesToAccel(e.payload.xvalue[2*i],e.payload.xvalue[2*i+1]));
        yarray.push(convertBytesToAccel(e.payload.yvalue[2*i],e.payload.yvalue[2*i+1]));
        zarray.push(convertBytesToAccel(e.payload.zvalue[2*i],e.payload.zvalue[2*i+1]));
        vectorSum.push(  Math.sqrt(Math.pow(xarray[i],2)  +  Math.pow(yarray[i],2)  +  Math.pow(zarray[i],2))  );
        mean += Math.sqrt(Math.pow(xarray[i],2)  +  Math.pow(yarray[i],2)  +  Math.pow(zarray[i],2));
      }
      
      //navigator.geolocation.getCurrentPosition(geolocationSuccess,[geolocationError]);
           
      
      //**************************************************** fall detection ******************************************************************
      if(possibleFall){
        if(peakIndex+15>=buffer_size){
          //auto simainei oti ola dedomena pou xreiazomaste einai
          //ston trexonta pinaka kai oxi ston overlappingRight.
          var m = peakIndex+15-buffer_size;
          for(var o=m;o<m+35;o++)
            fallDistance += Math.sqrt( Math.pow((xarray[o+1] - xarray[o]),2) + Math.pow((yarray[o+1] - yarray[o]),2) + Math.pow((zarray[o+1] - zarray[o]),2) );
            fallDistance = fallDistance / 35;
              console.log("fallDistance after impact was: "+fallDistance);
              if(fallDistance<=85.91){
                console.log("FALL DETECTED. CALL FOR HELP!!");
                fallDetected = true;
                numberOfFalls++;
              } 
        }
        else{
          //edw exoume kapoia dedomena ston proigoumeno pinaka
          //kai kapoia ston trexonta opote xreiazomaste ton overlapping
          var tempo = buffer_size - (peakIndex+15);
          for(var r=50-tempo;r<50;r++)
            fallDistance += overlappingVectorSumRight[r];
          for(r=0;r<35-tempo;r++)
            fallDistance += Math.sqrt( Math.pow((xarray[r+1] - xarray[r]),2) + Math.pow((yarray[r+1] - yarray[r]),2) + Math.pow((zarray[r+1] - zarray[r]),2) );
          fallDistance = fallDistance / 35;
          console.log("fallDistance after impact was: "+fallDistance);
          if(fallDistance<=85.91){
            console.log("FALL DETECTED. CALL FOR HELP!!");
            fallDetected = true;
            numberOfFalls++;
          } 
        }
      possibleFall = false; 
      }
      
      for(i=0;i<buffer_size;i++){
        if(vectorSum[i]>=3874.88){
          console.log("Possible Fall here cause of high vectorSum: "+vectorSum[i]);
          //check if there is a minimum <= 697.28 before the peak
          tempMinimum = vectorSum[i];
          if(i-15<0){
            //uses overlapping window 
            for(var k=i;k<15;k++){
              if (overlappingVectorSumLeft[k]<tempMinimum)
                tempMinimum = overlappingVectorSumLeft[k];
            }
            for(k=0;k<i;k++){
              if (vectorSum[k]<tempMinimum)
                tempMinimum = vectorSum[k];
            }
          }
          else{
            for(var j=i-15;j<i;j++){
              if (vectorSum[j]<tempMinimum)
                tempMinimum=vectorSum[j];
            }
          }
          console.log("minimum before peak was: "+tempMinimum);
          
          if(tempMinimum<=697.28){
              //possible fall, check for sleep/mild activity at (i+15,i+50)
            if(i+50>=buffer_size){   
              overlappingVectorSumRight.length = 0;
              for(var l=buffer_size-50;l<buffer_size;l++)
                overlappingVectorSumRight.push(Math.sqrt(Math.pow(xarray[l],2)  +  Math.pow(yarray[l],2)  +  Math.pow(zarray[l],2)) );
              possibleFall = true; //to check later
              peakIndex = i;
            }
            else{
              for(var p=i+15;p<i+50;p++)
                fallDistance += Math.sqrt( Math.pow((xarray[p+1] - xarray[p]),2) + Math.pow((yarray[p+1] - yarray[p]),2) + Math.pow((zarray[p+1] - zarray[p]),2) );
              fallDistance = fallDistance / 35;
              console.log("fallDistance after impact was: "+fallDistance);
              if(fallDistance<=85.91){
                console.log("FALL DETECTED. CALL FOR HELP!!");
                fallDetected = true;
                numberOfFalls++;
              } 
            }
          }
          
           
        }
      }
      
      overlappingVectorSumLeft.length = 0;
      //create overlapping window
      for(i=buffer_size-15;i<buffer_size;i++){
          overlappingVectorSumLeft.push(Math.sqrt(Math.pow(xarray[i],2)  +  Math.pow(yarray[i],2)  +  Math.pow(zarray[i],2)) );
      }
      
      if(fallDetected){
        Pebble.sendAppMessage({"status": 1}, messageSuccessHandler, messageFailureHandler);
        fallDetected = false;
      }
      
      
      //**************************************************** fall detection ******************************************************************
      
      for(i=0;i<time_samples;i++){
        var temp = convertBytesToTime(e.payload.timevalue[7*i],e.payload.timevalue[7*i+1],e.payload.timevalue[7*i+2],
                                  e.payload.timevalue[7*i+3],e.payload.timevalue[7*i+4],e.payload.timevalue[7*i+5],
                                  e.payload.timevalue[7*i+6]);
        console.log(temp);
        timearray.push(temp); 
      }
      
      mean = mean / buffer_size;
      for(i=0;i<buffer_size;i++){
        variance += Math.pow((vectorSum[i] - mean),2);
      }
      variance = variance / buffer_size;
      for(i=0;i<buffer_size-1;i++){
        meanCos += ( xarray[i]*xarray[i+1] + yarray[i]*yarray[i+1] + zarray[i]*zarray[i+1] ) / ( vectorSum[i] * vectorSum[i+1] );
        meanDistance += Math.sqrt( Math.pow((xarray[i+1] - xarray[i]),2) + Math.pow((yarray[i+1] - yarray[i]),2) + Math.pow((zarray[i+1] - zarray[i]),2) );
      }
      meanCos = meanCos / (buffer_size-1);
      meanDistance = meanDistance / (buffer_size-1);
      
      console.log("== meanDist: "+meanDistance+" == meanCos: "+meanCos+" == variance: "+variance);
      if(meanDistance <= 85.91){
        if(meanCos <= 0.99963){
          console.log("++++++++++++ MILD ++++++++++++");
          activityType.push("Mild");
          typeStart.push(timearray[0]);
          typeEnd.push(timearray[timearray.length-1]);
        }
        else{
          console.log("zzzzzzzZzZzZ SLEEP zzZZzZzZzZZZz");
          activityType.push("Sleep");
          typeStart.push(timearray[0]);
          typeEnd.push(timearray[timearray.length-1]);
        }
      }
      else{
        if(variance <= 233345.72){
          console.log("------------ MODERATE ---------------");
          activityType.push("Moderate");
          typeStart.push(timearray[0]);
          typeEnd.push(timearray[timearray.length-1]);
        }
        else{
          console.log("^^^^^^^^^^^^ INTENSE ^^^^^^^^^^^^^^^^");
          activityType.push("Intense");
          typeStart.push(timearray[0]);
          typeEnd.push(timearray[timearray.length-1]);
        }
      }
      
      xarray.length = 0;
      yarray.length = 0;
      zarray.length = 0;
      timearray.length = 0;
      
      console.log("numberOfMessages = "+numberOfMessages+" (sends data on server when numberOfMessages equals 120)");
      
      if(numberOfMessages==60){
        
        var fallsText = "Possible falls: " + numberOfFalls;
        sendData = {"user":username,"type":activityType,"start":typeStart,"end":typeEnd,"startTime":typeStart[0],"endTime":typeEnd[typeEnd.length-1],"comment":fallsText};     
        request.open('POST','http://83.212.115.163:8080/tracker',true);
        request.setRequestHeader('Content-Type', 'application/json; charset=utf-8');   
        request.send(JSON.stringify(sendData));
         
        numberOfMessages = 0;
        activityType.length = 0;
        typeStart.length = 0;
        typeEnd.length = 0;
        numberOfFalls = 0;
      }
      
    }
        
});