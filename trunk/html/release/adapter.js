var RTCPeerConnection=null;var getUserMedia=null;var attachMediaStream=null;var drawImage=null;var reattachMediaStream=null;var webrtcDetectedBrowser=null;var webrtcDetectedVersion=null;window.performance=window.performance||{};window.performance.now=window.performance.now||(function(){var a=Date.now();return function(){return Date.now()-a}})();function trace(a){if(a[a.length-1]=="\n"){a=a.substring(0,a.length-1)}console.log((performance.now()/1000).toFixed(3)+": "+a)}function maybeFixConfiguration(b){if(!b){return}for(var a=0;a<b.iceServers.length;a++){if(b.iceServers[a].hasOwnProperty("urls")){b.iceServers[a]["url"]=b.iceServers[a]["urls"];delete b.iceServers[a]["urls"]}}}drawImage=function(c,e,b,f,d,a){c.drawImage(e,b,f,d,a)};attachEventListener=function(c,b,d,a){c.addEventListener(b,d,a)};if(navigator.mozGetUserMedia){console.log("This appears to be Firefox");webrtcDetectedBrowser="firefox";webrtcDetectedVersion=parseInt(navigator.userAgent.match(/Firefox\/([0-9]+)\./)[1],10);var RTCPeerConnection=function(a,b){maybeFixConfiguration(a);return new mozRTCPeerConnection(a,b)};RTCSessionDescription=mozRTCSessionDescription;RTCIceCandidate=mozRTCIceCandidate;getUserMedia=navigator.mozGetUserMedia.bind(navigator);navigator.getUserMedia=getUserMedia;createIceServer=function(b,f,a){var d=null;var e=b.split(":");if(e[0].indexOf("stun")===0){d={url:b}}else{if(e[0].indexOf("turn")===0){if(webrtcDetectedVersion<27){var c=b.split("?");if(c.length===1||c[1].indexOf("transport=udp")===0){d={url:c[0],credential:a,username:f}}}else{d={url:b,credential:a,username:f}}}}return d};createIceServers=function(c,e,a){var d=[];for(i=0;i<c.length;i++){var b=createIceServer(c[i],e,a);if(b!==null){d.push(b)}}return d};attachMediaStream=function(a,b){console.log("Attaching media stream");a.mozSrcObject=b;a.play();return a};reattachMediaStream=function(b,a){console.log("Reattaching media stream");b.mozSrcObject=a.mozSrcObject;b.play()}}else{if(navigator.webkitGetUserMedia){console.log("This appears to be Chrome");webrtcDetectedBrowser="chrome";var result=navigator.userAgent.match(/Chrom(e|ium)\/([0-9]+)\./);if(result!==null){webrtcDetectedVersion=parseInt(result[2],10)}else{webrtcDetectedVersion=999}createIceServer=function(b,e,a){var c=null;var d=b.split(":");if(d[0].indexOf("stun")===0){c={url:b}}else{if(d[0].indexOf("turn")===0){c={url:b,credential:a,username:e}}}return c};createIceServers=function(c,e,a){var d=[];if(webrtcDetectedVersion>=34){d={urls:c,credential:a,username:e}}else{for(i=0;i<c.length;i++){var b=createIceServer(c[i],e,a);if(b!==null){d.push(b)}}}return d};var RTCPeerConnection=function(a,b){if(webrtcDetectedVersion<34){maybeFixConfiguration(a)}return new webkitRTCPeerConnection(a,b)};getUserMedia=navigator.webkitGetUserMedia.bind(navigator);navigator.getUserMedia=getUserMedia;attachMediaStream=function(a,b){if(typeof a.srcObject!=="undefined"){a.srcObject=b}else{if(typeof a.mozSrcObject!=="undefined"){a.mozSrcObject=b}else{if(typeof a.src!=="undefined"){a.src=URL.createObjectURL(b)}else{console.log("Error attaching stream to element.")}}}return a};reattachMediaStream=function(b,a){b.src=a.src}}else{var console=console||{log:function(a){}};var extractPluginObj=function(a){return a.isWebRtcPlugin?a:a.pluginObj};var attachEventListener=function(b,c,d,a){var e=extractPluginObj(b);if(e){e.bindEventListener(c,d,a)}else{if(typeof b.addEventListener!=="undefined"){b.addEventListener(c,d,a)}else{if(typeof b.addEvent!=="undefined"){b.addEventListener("on"+c,d,a)}}}};function getPlugin(){return document.getElementById("WebrtcEverywherePluginId")}var installPlugin=function(){if(document.getElementById("WebrtcEverywherePluginId")){return}console.log("installPlugin() called");var c=!!((Object.getOwnPropertyDescriptor&&Object.getOwnPropertyDescriptor(window,"ActiveXObject"))||("ActiveXObject" in window));var a=!!navigator.userAgent.indexOf("Safari");var b=document.createElement("object");if(c){b.setAttribute("classid","CLSID:7FD49E23-C8D7-4C4F-93A1-F7EACFA1EC53");c=true}else{b.setAttribute("type","application/webrtc-everywhere")}b.setAttribute("id","WebrtcEverywherePluginId");document.body.appendChild(b);b.setAttribute("width","0");b.setAttribute("height","0");if(b.isWebRtcPlugin||(typeof navigator.plugins!=="undefined"&&!!navigator.plugins["WebRTC Everywhere"])){console.log("Plugin version: "+b.versionName+", adapter version: 1.1.0");if(c){console.log("This appears to be Internet Explorer");webrtcDetectedBrowser="Internet Explorer"}else{if(a){console.log("This appears to be Safari");webrtcDetectedBrowser="Safari"}else{}}}else{console.log("Browser does not appear to be WebRTC-capable")}};if(document.body){installPlugin()}else{attachEventListener(window,"load",function(){console.log("onload");installPlugin()});attachEventListener(document,"readystatechange",function(){console.log("onreadystatechange:"+document.readyState);if(document.readyState=="complete"){installPlugin()}})}var getUserMediaDelayed;getUserMedia=navigator.getUserMedia=function(c,a,b){if(document.readyState!=="complete"){console.log("readyState = "+document.readyState+", delaying getUserMedia...");if(!getUserMediaDelayed){getUserMediaDelayed=true;attachEventListener(document,"readystatechange",function(){if(getUserMediaDelayed&&document.readyState=="complete"){getUserMediaDelayed=false;getPlugin().getUserMedia(c,a,b)}})}}else{getPlugin().getUserMedia(c,a,b)}};attachMediaStream=function(d,g){console.log("Attaching media stream");if(d.isWebRtcPlugin){d.src=g;return d}else{if(d.nodeName.toLowerCase()==="video"){if(!d.pluginObj&&g){var h=document.createElement("object");var c=(Object.getOwnPropertyDescriptor&&Object.getOwnPropertyDescriptor(window,"ActiveXObject"))||("ActiveXObject" in window);if(c){h.setAttribute("classid","CLSID:7FD49E23-C8D7-4C4F-93A1-F7EACFA1EC53")}else{h.setAttribute("type","application/webrtc-everywhere")}d.pluginObj=h;h.setAttribute("className",d.className);h.setAttribute("innerHTML",d.innerHTML);var e=d.getAttribute("width");var a=d.getAttribute("height");var f=d.getBoundingClientRect();if(!e){e=f.right-f.left}if(!a){a=f.bottom-f.top}if("getComputedStyle" in window){var b=window.getComputedStyle(d,null);if(!e&&b.width!="auto"&&b.width!="0px"){e=b.width}if(!a&&b.height!="auto"&&b.height!="0px"){a=b.height}}if(e){h.setAttribute("width",e)}else{h.setAttribute("autowidth",true)}if(a){h.setAttribute("height",a)}else{h.setAttribute("autoheight",true)}document.body.appendChild(h);if(d.parentNode){d.parentNode.replaceChild(h,d);document.body.appendChild(d);d.style.visibility="hidden"}}if(d.pluginObj){d.pluginObj.bindEventListener("play",function(j){if(d.pluginObj){if(d.pluginObj.getAttribute("autowidth")&&j.videoWidth){d.pluginObj.setAttribute("width",j.videoWidth)}if(d.pluginObj.getAttribute("autoheight")&&j.videoHeight){d.pluginObj.setAttribute("height",j.videoHeight)}}});d.pluginObj.src=g}return d.pluginObj}else{if(d.nodeName.toLowerCase()==="audio"){return d}}}};drawImage=function(d,b,h,g,c,k){var j=extractPluginObj(b);if(j&&j.isWebRtcPlugin&&j.videoWidth>0&&j.videoHeight>0){if(typeof j.getScreenShot!=="undefined"){var f=j.getScreenShot();if(f){var e=new Image();e.onload=function(){d.drawImage(e,0,0,c,k)};e.src="data:image/png;base64,"+f}}else{var a=d.createImageData(j.videoWidth,j.videoHeight);if(a){j.fillImageData(a);d.putImageData(a,h,g)}}}};MediaStreamTrack={};var getSourcesDelayed;MediaStreamTrack.getSources=function(a){if(document.readyState!=="complete"){console.log("readyState = "+document.readyState+", delaying getSources...");if(!getSourcesDelayed){getSourcesDelayed=true;attachEventListener(document,"readystatechange",function(){if(getSourcesDelayed&&document.readyState=="complete"){getSourcesDelayed=false;getPlugin().getSources(a)}})}}else{getPlugin().getSources(a)}};RTCPeerConnection=function(b,a){return getPlugin().createPeerConnection(b,a)};RTCIceCandidate=function(a){return getPlugin().createIceCandidate(a)};RTCSessionDescription=function(a){return getPlugin().createSessionDescription(a)}}};