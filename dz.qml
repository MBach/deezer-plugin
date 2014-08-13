import QtQuick 2.3
import QtWebKit 3.0
import QtQuick.Window 2.1
import "dz.js" as DeezerAPI

Window {
    id: window

    function helloDZ(msg) {

        var d = window.document;

        var s = '<li>text</li>';
        d.createElement(s);

        console.log(msg)

        /*DeezerAPI.DZ.init({appId:'141475', channelUrl:'http://mbach.github.io/Miam-Player/deezer-light/channel.html'});
        DeezerAPI.DZ.getLoginStatus(function(response) {
        if (response.authResponse) {
         console.log("you are logged")
        } else {
         console.log("no session :(")
        }
        });*/

        return "helloDZ !"
    }
 }
