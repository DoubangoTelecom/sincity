var RTCPeerConnection = window.RTCPeerConnection || window.mozRTCPeerConnection || window.webkitRTCPeerConnection;
var RTCSessionDescription = window.RTCSessionDescription || window.mozRTCSessionDescription || window.webkitRTCSessionDescription;
var RTCIceCandidate = window.RTCIceCandidate || window.mozRTCIceCandidate || window.webkitRTCIceCandidate;
var WebSocket = (window['MozWebSocket'] || window['MozWebSocket'] || WebSocket);

var SCEngine = {
    onconnected: null,
    ondisconnected: null,
    oncall: null,
    
    socket: null,
    config: null,
    connected: false
};
var SCCall = function (){
    /*from: null,
    to: null,
    cid: null,
    tid: null,
    pc: null,
    remote_video:null,
    pendingOffer:null*/
};
SCCall.prototype.call = function() {
    if (!SCEngine.connected) {
        throw new Error("not connected"); 
    }
    if (this.pc) {
        throw new Error("already connected"); 
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
        var remoteVideo = (call.remote_video || SCEngine.config.remote_video);
        if (remoteVideo) {
            remoteVideo.src = webkitURL.createObjectURL(e.stream);
        }
        SCUtils.raiseCallEvent(call, "info", "media started!");
    }
};

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

SCEngine.init = function(config) {
    SCEngine.config = config;
}

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
            if (msg.from == SCEngine.config.local_id) {
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

SCEngine.disconnect = function() {
    if (SCEngine.socket) {
        SCEngine.socket.close();
        SCEngine.socket = null;
    }
    return true;
}

SCEngine.newCall = function(dest) {
    var newCall;
    if (typeof dest == "string") { 
        newCall = new SCCall();
        newCall.from = SCEngine.config.local_id;
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
