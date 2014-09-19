/**
@fileoverview This is SinCity WebRTC "library".

@name        libsincity
@author      Doubango Telecom
@version     1.0.0
*/

var RTCPeerConnection = window.RTCPeerConnection || window.mozRTCPeerConnection || window.webkitRTCPeerConnection;
var RTCSessionDescription = window.RTCSessionDescription || window.mozRTCSessionDescription || window.webkitRTCSessionDescription;
var RTCIceCandidate = window.RTCIceCandidate || window.mozRTCIceCandidate || window.webkitRTCIceCandidate;
var WebSocket = (window['MozWebSocket'] || window['MozWebSocket'] || WebSocket);

/**
@namespace
@description Engine
*/
var SCEngine = {
    /**
    Raised when the engine is connected to the signaling server.
    @event
    */
    onconnected: null,
    /**
    Raised when the engine is disconnected to the signaling server.
    @event
    */
    ondisconnected: null,
     /**
    Raised to signal an event related to a call session.
    @event
    @param {SCEventCall} e The event.
    */
    oncall: null,
    
    /** @private */socket: null,
    /** @private */config: null,
    /** @private */connected: false
};
/**
@namespace
@description Call session
*/
var SCCall = function (){
    /*from: null,
    to: null,
    cid: null,
    tid: null,
    pc: null,
    remoteVideo:null,
    pendingOffer:null*/
};
/**
Makes an outgoing call.
@returns {Boolean} <i>true</i> if succeed; otherwise <i>false</i>
@throws {ERR_NOT_CONNECTED}
@throws {ERR_NOT_ALREADY_INCALL}
*/
SCCall.prototype.call = function() {
    if (!SCEngine.connected) {
        throw new Error("not connected"); 
    }
    if (this.pc) {
        throw new Error("already in call"); 
    }
    
    var This = this;
    this.pc = new RTCPeerConnection(SCUtils.buildPeerConnConfig());
    this.pc.onicecandidate  = function(e) { SCWebRtcEvents.onicecandidate(This, e); }
    this.pc.onaddstream = function(e) { SCWebRtcEvents.onaddstream(This, e); }
    this.pc.createOffer(
        function(desc) {
            console.info("createOffer:" + desc.sdp);
            This.pc.setLocalDescription(desc, 
                function() { 
                    console.info("setLocalDescription OK");
                    SCUtils.raiseCallEvent(This, "info", "gathering ICE candidates...");
                },
                function(err) {
                    console.error("setLocalDescription:" + err);
                    SCUtils.raiseCallEvent(This, "error", err);
                });
        },
        function(err) {
            console.error("createOffer:" + err);
            SCUtils.raiseCallEvent(This, "error", err);
        },
        SCUtils.buildSdpConstraints());
    
    return true;
}

/**
Accepts an incoming message received from the signaling server.
@param {SCMessage} message The message to accept.
@throws {ERR_NOT_CONNECTED}
@throws {ERR_NOT_NOT_IMPLEMENTED}
*/
SCCall.prototype.acceptMsg = function(msg) {
    if (!SCEngine.connected) {
        throw new Error("not connected"); 
    }
    var This = this;
    if (msg.type === "answer" || msg.type === "pranswer") {
        var sdpRemote = msg.sdp.replace("UDP/TLS/RTP/SAVPF", "RTP/SAVPF");
        this.pc.setRemoteDescription(new RTCSessionDescription({ type: msg.type, sdp: sdpRemote }), 
            function() { 
                console.info("setRemoteDescription OK");
            },
            function(err) { 
                console.error("setRemoteDescription NOK:" + err); 
                SCUtils.raiseCallEvent(This, "error", err);
            }
        );
    }
    else if (msg.type === "offer") {
        var This = this;
        if (!this.pc) {
            this.pc = new RTCPeerConnection(SCUtils.buildPeerConnConfig());
            this.pc.onicecandidate  = function(e) { SCWebRtcEvents.onicecandidate(This, e); }
            this.pc.onaddstream = function(e) { SCWebRtcEvents.onaddstream(This, e); }
        }
        var sdpRemote = msg.sdp.replace("UDP/TLS/RTP/SAVPF", "RTP/SAVPF");
        this.pc.setRemoteDescription(new RTCSessionDescription({ type: "offer", sdp: sdpRemote }), 
            function() { 
                console.info("setRemoteDescription OK");
                This.pc.createAnswer(
                    function(desc) {
                        console.info("createAnswer:" + desc.sdp);
                        This.pc.setLocalDescription(desc, 
                            function() { 
                                console.info("setLocalDescription OK");
                                SCUtils.raiseCallEvent(This, "info", "gathering ICE candidates...");
                            },
                            function(err) {
                                console.error("setLocalDescription:" + err);
                                SCUtils.raiseCallEvent(This, "error", err);
                            });
                    },
                    function(err) {
                        console.info("createAnswer:" + err);
                        SCUtils.raiseCallEvent(This, "error", err);
                    },
                    SCUtils.buildSdpConstraints());
            },
            function(err) { 
                console.info("setRemoteDescription NOK:" + err);
                SCUtils.raiseCallEvent(This, "error", err);
            }
        );
    }
    else if (msg.type === "hangup") {
        if(This.pc) {
            This.pc.close();
            This.pc = null;
        }
    }
    else {
        throw new Error("not implemented yet");
    }
    return true;
}
/**
Rejects an incoming message received from the signaling server.
@param {SCMessage} message The message to reject.
@throws {ERR_NOT_CONNECTED}
*/
SCCall.rejectMsg = function(msg) {
    if (!SCEngine.connected) {
        throw new Error("not connected"); 
    }
    var msg_text = JSON.stringify({
        from: msg.to,
        to: msg.from,
        cid: msg.cid,
        tid: msg.tid,
        type: "hangup"
    });
    console.info("send: " + msg_text);
    SCEngine.socket.send(msg_text);
    return true;
}
/**
Terminates the call session.
@throws {ERR_NOT_CONNECTED}
*/
SCCall.prototype.hangup = function() {
    if(this.pc) {
        this.pc.close();
        this.pc = null;
    }
    if (!SCEngine.connected) {
        throw new Error("not connected"); 
    }
    var msg_text = JSON.stringify({
        from: this.from,
        to: this.to,
        cid: this.cid,
        tid: SCUtils.stringRandomUuid(),
        type: "hangup"
    });
    console.info("send: " + msg_text);
    SCEngine.socket.send(msg_text);
    return true;
}

/**
Builds a new <a href="SCCall.html">SCCall</a> object.
@param {String / SCMessage} dest Could be a <b>String</b> or a <a href="SCMessage.html">SCMessage</a> object with <a href="SCMessage.html#type">type</a> equal to "offer". <br />
The <b>String</b> verstion is for outgoing calls while the <a href="SCMessage.html">SCMessage</a> version is for incoming calls.
@returns {SCCall} new call session.
*/
SCCall.build = function(dest) {
    var newCall;
    if (typeof dest == "string") { 
        newCall = new SCCall();
        newCall.from = SCEngine.config.localId;
        newCall.to = dest;
        newCall.cid = SCUtils.stringRandomUuid();
        newCall.tid = SCUtils.stringRandomUuid();
    }
    else if (dest.type == "offer"){
        newCall = new SCCall();
        newCall.from = dest.to;
        newCall.to = dest.from;
        newCall.cid = dest.cid;
        newCall.tid = dest.tid;
    }
    return newCall;
}

/** @private */
var SCWebRtcEvents = {
    onicecandidate: function(call, e) {
        var This = call;
        if (e.candidate) { 
            call.pc.addIceCandidate(new RTCIceCandidate(e.candidate),
                function() { console.info("addIceCandidate OK"); },
                function(err) { 
                    console.error("addIceCandidate NOK:" + err); 
                    SCUtils.raiseCallEvent(This, "error", err);
                }
            );
        }
        else {
            if (!call.pc) {
                // closing the peerconnection could raise this event with null candidate --> ignore
                console.info("ICE gathering done...but pc is null");
                return;
            }
            console.info("ICE gathering done");
            SCUtils.raiseCallEvent(This, "info", "gathering ICE candidates done!");
            if (!SCEngine.connected) {
                SCUtils.raiseCallEvent(This, "error", "ICE gathering done but signaling transport not connected");
                return;
            }
            var msg = 
            {
                from: call.from,
                to: call.to,
                cid: call.cid,
                tid: call.tid,
                type: call.pc.localDescription.type, // "offer"
                sdp: call.pc.localDescription.sdp.replace("RTP/SAVPF", "UDP/TLS/RTP/SAVPF")
            };
            var msg_text = JSON.stringify(msg);
            console.info("send: " + msg_text);
            SCEngine.socket.send(msg_text);
        }
    },
    onaddstream: function(call, e) {
        var remoteVideo = (call.remoteVideo || SCEngine.config.remoteVideo);
        if (remoteVideo) {
            if (window.webkitURL) { // Chrome
                remoteVideo.src = webkitURL.createObjectURL(e.stream);
            }
            else if (remoteVideo.mozSrcObject) { // FF
                remoteVideo.mozSrcObject = e.stream;
                remoteVideo.play();
            }
        }
        SCUtils.raiseCallEvent(call, "info", "media started!");
    }
};

/** @private */
var SCUtils = {
    stringFormat: function(s_str) {
        for (var i = 1; i < arguments.length; i++) {
            var regexp = new RegExp('\\{' + (i - 1) + '\\}', 'gi');
            s_str = s_str.replace(regexp, arguments[i]);
        }
        return s_str;
    },
    stringRandomFromDict: function(i_length, s_dict) {
        var s_ret = "";
        for (var i = 0; i < i_length; i++) {
            s_ret += s_dict[Math.floor(Math.random() * s_dict.length)];
        }
        return s_ret;
    },
    stringRandom: function(i_length) {
        var s_dict = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXTZabcdefghiklmnopqrstuvwxyz";
        return SCUtils.stringRandomFromDict(i_length, s_dict);
    },
    stringRandomUuid: function() {
        // e.g. 6ba7b810-9dad-11d1-80b4-00c04fd430c8
        var s_dict = "0123456789abcdef";
        return SCUtils.stringFormat("{0}-{1}-{2}-{3}-{4}",
                SCUtils.stringRandomFromDict(8, s_dict),
                SCUtils.stringRandomFromDict(4, s_dict),
                SCUtils.stringRandomFromDict(4, s_dict), 
                SCUtils.stringRandomFromDict(4, s_dict),
                SCUtils.stringRandomFromDict(12, s_dict));
    },
    buildPeerConnConfig: function() {
        return {
            iceServers: SCEngine.config.iceServers
        };
    },
    buildSdpConstraints: function() {
        return {
              'mandatory': {
                'OfferToReceiveAudio': false,
                'OfferToReceiveVideo': true
              }
        };
    },
    raiseCallEvent: function(call, type, description, msg) {
        if (SCEngine.oncall) {
            SCEngine.oncall({ "type": type, "call": call, "description": description, "msg": msg});
        }
    },
};

/**
Initialize the engine. Must be the first function to call.
@param {SCConfigEngine} config
*/
SCEngine.init = function(config) {
    SCEngine.config = config;
}

/**
Connects to the signaling server. Must be called before making any call.
@param {String} url A WebSocket URL.e.g. "ws://localhost:9000/wsStringStaticMulti?roomId=0"
@throws {ERR_ALREADY_CONNECTED}
*/
SCEngine.connect = function(url) {
    console.info("connecting to: " + url);
    if (SCEngine.socket) {
        throw new Error("already connected"); 
    }
    
    SCEngine.socket = new WebSocket(url, "ge-webrtc-signaling");
    SCEngine.socket.onopen = function() {
        console.info("onopen");
        if (SCEngine.onconnected) {
            SCEngine.onconnected();
        }
        SCEngine.connected = true;
    }
    SCEngine.socket.onclose = function() {
        console.info("onclose");
        if (SCEngine.ondisconnected) {
            SCEngine.ondisconnected();
        }
        SCEngine.connected = false;
        SCEngine.socket = null;
    }
    SCEngine.socket.onmessage = function(e) {
        console.info("onmessage: " + e.data);
        
        var msg = JSON.parse(e.data);
        if (msg)
        {
            if (msg.from == SCEngine.config.localId) {
                console.info("message loopback for " + msg.from);
                return;
            }
            if (msg.type === "offer" || msg.type === "answer" || msg.type == "pranswer" || msg.type == "hangup") {
                SCUtils.raiseCallEvent(null, msg.type, msg.type, msg);
            }
        }
    }
    return true;
}

/**
Disconnect the engine from the signaling server.
*/
SCEngine.disconnect = function() {
    if (SCEngine.socket) {
        SCEngine.socket.close();
        SCEngine.socket = null;
    }
    return true;
}

/**
JSON message from the server.
@namespace SCMessage
@name SCMessage
@property {String} type The message type. Possible values: "offer", "answer", "pranswer" or "hangup". <b>Required</b>.
@property {String} to The destination identifer. <b>Required</b>.
@property {String} from The source identifier. <b>Required</b>.
@property {String} cid The call (dialog) id. <b>Required</b>.
@property {String} tid The transaction id. <b>Required</b>.
@property {String} sdp The session description. <i>Optional</i>.
*/

/**
Event object.
@namespace SCEventCall
@name SCEventCall
@property {String} type The event type. Possible values: "offer", "answer", "pranswer", "hangup", "error" or "info".
@property {SCCall} call The call object associated to this event.
@property {String} description The event description.
@property {SCMessage} msg The message associated to this event.
*/

/**
Engine configuration object.
@namespace SCConfigEngine
@name SCConfigEngine
@property {String} localId Local user/device identifier.
@property {HTMLVideoElement} [remoteVideo] <a href="https://developer.mozilla.org/en-US/docs/DOM/HTMLVideoElement">HTMLVideoElement<a> where to display the remote video stream.
@property {Array} iceServers Array of STUN/TURN servers to use. The format is as explained at <a href="http://www.w3.org/TR/webrtc/#rtciceserver-type" target=_blank>http://www.w3.org/TR/webrtc/#rtciceserver-type</a> <br />
Example: [{ url: 'stun:stun.l.google.com:19302'}, { url:'turn:user@numb.viagenie.ca', credential:'myPassword'}] 
*/
