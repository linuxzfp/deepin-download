!function e(r, t, n) {
    function o(a, d) {
        if (!t[a]) {
            if (!r[a]) {
                var u = "function" == typeof require && require;
                if (!d && u) return u(a, !0);
                if (i) return i(a, !0);
                var c = new Error("Cannot find module '" + a + "'");
                throw c.code = "MODULE_NOT_FOUND", c
            }
            var s = t[a] = {exports: {}};
            r[a][0].call(s.exports, function (e) {
                var t = r[a][1][e];
                return o(t || e)
            }, s, s.exports, e, r, t, n)
        }
        return t[a].exports
    }

    for (var i = "function" == typeof require && require, a = 0; a < n.length; a++) o(n[a]);
    return o
}({
    1: [function (e, r, t) {
        "use strict";

        function n(e) {
            chrome.runtime.sendMessage({method: "addScript", data: "js/" + e + ".js"})
        }

        function o() {
            var e = document.createElement("script");
            e.src = chrome.runtime.getURL("js/baidu.js"), document.body.appendChild(e)
        }

        "undefined" != typeof browser && (chrome = browser), window.addEventListener("message", function (e) {
            "yunData" === e.data.type && (window.yunData = e.data.data, n(window.location.href.includes("/disk/home") ? "home" : "share"))
        }), "complete" === document.readyState ? o() : window.addEventListener("load", o)
    }, {}]
}, {}, [1]);