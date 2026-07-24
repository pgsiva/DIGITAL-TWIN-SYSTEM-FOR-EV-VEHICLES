// Time

function updateClock(){

const now=new Date();

const options={
weekday:'long',
day:'numeric',
month:'long',
year:'numeric'
};

document.getElementById("date").innerHTML=now.toLocaleDateString('en-IN',options);

document.getElementById("time").innerHTML=now.toLocaleTimeString();

let hour=now.getHours();

let greeting="";

if(hour<12){
greeting="Good Morning";
}
else if(hour<17){
greeting="Good Afternoon";
}
else{
greeting="Good Evening";
}

document.getElementById("greeting").innerHTML=greeting+" Bro!";
}

setInterval(updateClock,1000);

updateClock();




// App Opening

function openApp(app){

switch(app){

case "youtube":
window.open("https://youtube.com");
break;

case "whatsapp":
window.open("https://web.whatsapp.com");
break;

case "maps":
window.open("https://maps.google.com");
break;

case "spotify":
window.open("https://spotify.com");
break;

default:
alert(app+" clicked");
}

}