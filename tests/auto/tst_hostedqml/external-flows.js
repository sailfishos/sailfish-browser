/* SPDX-License-Identifier: MPL-2.0 */
var tabs = [
    {tabId:"1",persistentId:"11",openerId:"",locationRevision:"2"},
    {tabId:"2",persistentId:"12",openerId:"1",locationRevision:"5"},
    {tabId:"3",persistentId:"13",openerId:"",locationRevision:"1"},
    {tabId:"4",persistentId:"14",openerId:"2",locationRevision:"1"}
];
var closed = [], responses = [], cancelled = [];
var contentItem = {tabModel:{snapshot:function(){return tabs;}},
    sendAsyncMessageToTab:function(tab,topic,data){responses.push({tab:tab,topic:topic,data:data});}};
var webView = {closeTab:function(id){closed.push(id);}};
var handler = {cancel:function(id){cancelled.push(id);}};
var expiryTimer = {stop:function(){}};
var requests = {};
var request = {id:"r",tabId:"2",locationRevision:"5",winId:7,fallback:"https://example.com/fallback"};
check(current(request), "Live popup request must be current");
closeFlow(request);
deepEqual(closed,[14,12,11], "Callback closes popup tree and root, preserving concurrent flow 13");
requests.r = request;
reject(request);
equal(responses[0].data.outcome,"fallback", "Browser rejection follows fallback");
equal(responses[0].data.fallback,request.fallback);
equal(responses[0].tab,"2", "Fallback stays with originating popup");
requests.r = request;
tabs[1].locationRevision = "6";
reject(request);
equal(responses[1].data.outcome,"cancelled", "Stale query cannot navigate fallback");
equal(responses[1].data.fallback,"");
tabs.splice(1,1);
check(!current(request), "Closed popup request must be cancelled");
var before = responses.length;
respond(request,"dispatched");
equal(responses.length,before, "A completed request cannot dispatch twice");
var automatic = false, prompt = null, promptRequest = null, promptComponent = {};
var promptQueue = [], promptArguments;
var pageStack = {busy:false, animatorPush:function(component, args) {
    promptArguments = args;
    return {pageCompleted:{connect:function(){}}};
}};
request.tabId = "1";
request.locationRevision = "2";
request.canRemember = false;
requests.r = request;
promptQueue.push(request);
nextPrompt();
equal(promptArguments.canRemember, false, "Intent prompt must not offer Remember");
prompt = null;
request.canRemember = true;
promptQueue.push(request);
nextPrompt();
equal(promptArguments.canRemember, true, "Eligible custom protocols retain Remember");
true;
