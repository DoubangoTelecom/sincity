/**
@fileoverview This is SinCity WebRTC "library".

@name        libsincity
@author      Doubango Telecom
@version     1.3.0
*/
document.write(unescape("%3Cscript src='adapter.js' type='text/javascript'%3E%3C/script%3E"));
var WebSocket = (window['MozWebSocket'] || window['MozWebSocket'] || WebSocket);
window.console = window.console || {};
window.console.info = window.console.info || window.console.log || function(msg) { };
window.console.error = window.console.error || window.console.log || window.console.info;

/**
@namespace Anonymous
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
@namespace Anonymous
@description Call session
*/
var SCCall = function (){
    /*from: null,
    to: null,
    cid: null,
    tid: null,
    pc: null,
    config: null,
    localStream: null,
    remoteVideo:null,
    pendingOffer:null*/
    this.config = 
    { 
        audio_send: false, 
        audio_recv: false,
        audio_remote_elt: null,
        video_send: false,
        video_recv: true,
        video_local_elt: null,
        video_remote_elt: null
    };
};



/**
Sets the call config.
@param {SCConfigCall} config Call config
*/
SCCall.prototype.setConfig = function(config) {
    if (config) {    
        this.config.audio_send = (config.audio_send !== undefined) ? config.audio_send : false;
        this.config.audio_recv = (config.audio_recv !== undefined) ? config.audio_recv : false;
        this.config.audio_remote_elt = (config.audio_remote_elt || SCEngine.config.remoteAudio);
        this.config.video_send = (config.video_send !== undefined) ? config.video_send : false;
        this.config.video_recv = (config.video_recv !== undefined) ? config.video_recv : true;
        this.config.video_local_elt = (config.video_local_elt || SCEngine.config.localVideo);
        this.config.video_remote_elt = (config.video_remote_elt || SCEngine.config.remoteVideo);
    }
}

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
    if (this.config.audio_send || this.config.video_send) {
        console.info("getUserMedia(audio:" + this.config.audio_send + ", video:" + this.config.video_send + ")");
        getUserMedia(
                SCUtils.buildConstraintsGUM(this.config), 
                function(stream) { 
                    console.info("getUserMedia OK");
                    SCWebRtcEvents.ongotstream(This, stream);
                    SCUtils.makeOffer(This); // send offer
                },
                function (err) { 
                    console.error("getUserMedia:" + err);
                    SCUtils.raiseCallEvent(This, "error", err);
                }
            );
    }
    else {
        SCUtils.makeOffer(This); // send offer
    }
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
        var sdpRemote = msg.sdp.replace(/UDP\/TLS\/RTP\/SAVPF/g, 'RTP/SAVPF');
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
        var Msg = msg;
        if (this.config.audio_send || this.config.video_send) {
            console.info("getUserMedia(audio:" + this.config.audio_send + ", video:" + this.config.video_send + ")");
            getUserMedia(
                SCUtils.buildConstraintsGUM(this.config),
                function(stream) {
                    console.info("getUserMedia OK");
                    SCWebRtcEvents.ongotstream(This, stream);
                    SCUtils.makeAnswer(This, Msg); // send answer
                },
                function(err) {
                    console.error("getUserMedia:" + err);
                    SCUtils.raiseCallEvent(This, "error", err);
                }
            );
        }
        else {
            SCUtils.makeAnswer(This, Msg); // send answer
        }
    }
    else if (msg.type === "hangup") {
        SCUtils.closeMedia(This);
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
    SCUtils.closeMedia(this);
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
@param {String / SCMessage} dest Could be a <b>String</b> or a <a href="SCMessage.html">SCMessage</a> object with <a href="SCMessage.html#type">type</a> equal to "offer".
@param {SCConfigCall} [config] Call config <br />
The <b>String</b> verstion is for outgoing calls while the <a href="SCMessage.html">SCMessage</a> version is for incoming calls.
@returns {SCCall} new call session.
*/
SCCall.build = function(dest, config) {
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
    if (newCall && config) {
        newCall.setConfig(config);
    }
    return newCall;
}

/** @private */
var SCWebRtcEvents = {
    onicecandidate: function(call, e) {
        var This = call;
        if (e.candidate) {
            /*call.pc.addIceCandidate(new RTCIceCandidate(e.candidate),
                function() { console.info("addIceCandidate OK"); },
                function(err) {
                    console.error("addIceCandidate NOK:" + err);
                    SCUtils.raiseCallEvent(This, "error", err);
                }
            );*/
        }
        if (!e.candidate || (call.pc && (call.pc.iceState === "completed" || call.pc.iceGatheringState === "completed"))) {
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
                type: call.pc.localDescription.type, // "offer"/"answer"/"pranswer"
                sdp: SCUtils.stringLocalSdp(call.pc)
            };
            var msg_text = JSON.stringify(msg);
            console.info("send: " + msg_text);
            SCEngine.socket.send(msg_text);
        }
    },
    onaddstream: function(call, e) { // remote stream
        SCUtils.attachStream(call, e.stream, false/*remote*/);
        SCUtils.raiseCallEvent(call, "info", "media started!");
    },
    ongotstream: function(call, stream) { // local stream
        call.localStream = stream;
        if (call.pc) {
            call.pc.addStream(stream);
            SCUtils.attachStream(call, stream, true/*local*/);
        }
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
    stringLocalSdp: function(pc) {
        if (pc && pc.localDescription && pc.localDescription.sdp) {
            var sdp = pc.localDescription.sdp.replace(/RTP\/SAVPF/g, 'UDP/TLS/RTP/SAVPF');
            var arrayPosOfVideo = [];
            var posOfVideo, indexOfVideoStart, indexOfVideo, indexOfVideoNext, indexOfContentSlides;
            // Find video positions to consider as "screencast"
            if (pc.remoteDescription && pc.remoteDescription.sdp) {
                posOfVideo = 0;
                indexOfVideoStart = 0;
                while ((indexOfVideo = pc.remoteDescription.sdp.indexOf("m=video ", indexOfVideoStart)) > 0) {
                    if ((indexOfContentSlides = pc.remoteDescription.sdp.indexOf("a=content:slides", indexOfVideo)) > 0 || (indexOfContentSlides = pc.remoteDescription.sdp.indexOf("label:doubango@bfcpvideo", indexOfVideo)) > 0) {
                        indexOfVideoNext = pc.remoteDescription.sdp.indexOf("m=video ", indexOfVideo + 1);
                        if (indexOfVideoNext == -1 || indexOfContentSlides < indexOfVideoNext) {
                            // Video at "posOfVideo" is "screencast"
                            arrayPosOfVideo.push(posOfVideo);
                        }
                    }
                    indexOfVideoStart = indexOfVideo + 1;
                    ++posOfVideo;
                }
            }
            // Patch the SDP to have it looks like "screencast"
            for (var i = 0; i < arrayPosOfVideo.length; ++i) {
                posOfVideo = 0;
                indexOfVideoStart = 0;
                while ((indexOfVideo = sdp.indexOf("m=video ", indexOfVideoStart)) > 0) {
                    if (posOfVideo == arrayPosOfVideo[i]) {
                        var endOfLine = sdp.indexOf("\r\n", indexOfVideo);
                        if (endOfLine > 0) {
                            sdp = sdp.substring(0, (endOfLine + 2)) + "a=content:slides\r\n" + sdp.substring((endOfLine + 2), sdp.length);
                        }
                    }
                    indexOfVideoStart = indexOfVideo + 1;
                    ++posOfVideo;
                }
            }

            return sdp;
        }
    },
    buildPeerConnConfig: function() {
        return {
            iceServers: SCEngine.config.iceServers
        };
    },
    buildConstraintsSDP: function(callConfig) {
        return {
            'mandatory': {
                'OfferToReceiveAudio': (callConfig && callConfig.audio_recv !== undefined) ? callConfig.audio_recv : false,
                'OfferToReceiveVideo': (callConfig && callConfig.video_recv !== undefined) ? callConfig.video_recv : true
            }
        };
    },
    buildConstraintsGUM: function(callConfig) {
        return {
            'audio': (callConfig && callConfig.audio_send),
            'video': (callConfig && callConfig.video_send)
        };
    },
    attachStream: function(call, stream, local) {
        var htmlElementVideo = local ? (call.config.video_local_elt || SCEngine.config.localVideo) : (call.config.video_remote_elt || SCEngine.config.remoteVideo);
        var htmlElementAudio = local ? (call.config.audio_local_elt || SCEngine.config.localAudio) : (call.config.audio_remote_elt || SCEngine.config.remoteAudio);

        if (htmlElementAudio) {
            attachMediaStream(htmlElementAudio, stream);
        }
        if (htmlElementVideo) {
            attachMediaStream(htmlElementVideo, stream);
        }
    },
    closeMedia: function(call) {
        if (call) {
            if (call.pc) {
                if (call.localStream) {
                    try { call.pc.removeStream(call.localStream); } catch (e) { }
                }
                call.pc.close();
                call.pc = null;
            }
            if (call.localStream) {
                call.localStream.stop();
                call.localStream = null;
            }

            SCUtils.attachStream(call, null, false/*remote*/);
            SCUtils.attachStream(call, null, true/*local*/);
        }
    },
    makeOffer: function(call) {
        var This = call;
        if (!call.pc) {
            call.pc = new RTCPeerConnection(SCUtils.buildPeerConnConfig());
            if (call.localStream) {
                call.pc.addStream(call.localStream);
                SCUtils.attachStream(call, call.localStream, true/*local*/);
            }
        }
        call.pc.onicecandidate = function(e) { SCWebRtcEvents.onicecandidate(This, e); }
        call.pc.onaddstream = function(e) { SCWebRtcEvents.onaddstream(This, e); }
        call.pc.createOffer(
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
            SCUtils.buildConstraintsSDP(This.config));
    },
    makeAnswer: function(call, Msg) {
        var This = call;
        if (!call.pc) {
            call.pc = new RTCPeerConnection(SCUtils.buildPeerConnConfig());
            if (call.localStream) {
                call.pc.addStream(call.localStream);
                SCUtils.attachStream(call, call.localStream, true/*local*/);
            }
        }
        call.pc.onicecandidate = function(e) { SCWebRtcEvents.onicecandidate(This, e); }
        call.pc.onaddstream = function(e) { SCWebRtcEvents.onaddstream(This, e); }
        var sdpRemote = Msg.sdp.replace(/UDP\/TLS\/RTP\/SAVPF/g, 'RTP/SAVPF');
        call.pc.setRemoteDescription(new RTCSessionDescription({ type: "offer", sdp: sdpRemote }),
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
                    SCUtils.buildConstraintsSDP(This.config));
            },
            function(err) {
                console.info("setRemoteDescription NOK:" + err);
                SCUtils.raiseCallEvent(This, "error", err);
            }
        );
    },
    raiseCallEvent: function(call, type, description, msg) {
        if (SCEngine.oncall) {
            SCEngine.oncall({ "type": type, "call": call, "description": description, "msg": msg });
        }
    }
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
@namespace Anonymous
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
@namespace Anonymous
@name SCEventCall
@property {String} type The event type. Possible values: "offer", "answer", "pranswer", "hangup", "error" or "info".
@property {SCCall} call The call object associated to this event.
@property {String} description The event description.
@property {SCMessage} msg The message associated to this event.
*/

/**
Engine configuration object.
@namespace Anonymous
@name SCConfigEngine
@property {String} localId Local user/device identifier.
@property {HTMLVideoElement} [localVideo] <a href="https://developer.mozilla.org/en-US/docs/DOM/HTMLVideoElement">HTMLVideoElement<a> where to display the local video stream.
@property {HTMLVideoElement} [remoteVideo] <a href="https://developer.mozilla.org/en-US/docs/DOM/HTMLVideoElement">HTMLVideoElement<a> where to display the remote video stream.
@property {HTMLAudioElement} [remoteAudio] <a href="https://developer.mozilla.org/en-US/docs/DOM/HTMLAudioElement">HTMLAudioElement<a> used to playback the remote audio stream.
@property {Array} iceServers Array of STUN/TURN servers to use. The format is as explained at <a href="http://www.w3.org/TR/webrtc/#rtciceserver-type" target=_blank>http://www.w3.org/TR/webrtc/#rtciceserver-type</a> <br />
Example: [{ url: 'stun:stun.l.google.com:19302'}, { url:'turn:user@numb.viagenie.ca', credential:'myPassword'}] 
*/

/**
Call configuration object.
@namespace Anonymous
@name SCConfigCall
@property {Boolean} [audio_send] Defines whether to send audio. Default value: <b>false</b>.
@property {Boolean} [audio_recv] Defines whether to accept incoming audio. Default value: <b>false</b>.
@property {HTMLAudioElement} [audio_remote_elt] <a href="https://developer.mozilla.org/en-US/docs/DOM/HTMLAudioElement">HTMLAudioElement<a> used to playback the remote audio stream.
@property {Boolean} [video_send] Defines whether to send video. Default value: <b>false</b>.
@property {Boolean} [video_recv] Defines whether to accept incoming video.  Default value: <b>true</b>.
@property {HTMLVideoElement} [video_local_elt] <a href="https://developer.mozilla.org/en-US/docs/DOM/HTMLVideoElement">HTMLVideoElement<a> where to display the local video stream.
@property {HTMLVideoElement} [video_remote_elt] <a href="https://developer.mozilla.org/en-US/docs/DOM/HTMLVideoElement">HTMLVideoElement<a> where to display the remote video stream.<br />
Example: { audio_send: true, audio_recv: true, video_send: false, video_recv: true }
*/
