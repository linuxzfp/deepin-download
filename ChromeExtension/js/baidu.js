!function t(e, n, a) {
    function o(r, s) {
        if (!n[r]) {
            if (!e[r]) {
                var u = "function" == typeof require && require;
                if (!s && u) return u(r, !0);
                if (i) return i(r, !0);
                var c = new Error("Cannot find module '" + r + "'");
                throw c.code = "MODULE_NOT_FOUND", c
            }
            var f = n[r] = {exports: {}};
            e[r][0].call(f.exports, function (t) {
                var n = e[r][1][t];
                return o(n || t)
            }, f, f.exports, t, e, n, a)
        }
        return n[r].exports
    }

    for (var i = "function" == typeof require && require, r = 0; r < a.length; r++) o(a[r]);
    return o
}({
    1: [function (t, e, n) {
        "use strict";
        var a = function () {
            function t(t, e) {
                for (var n = 0; n < e.length; n++) {
                    var a = e[n];
                    a.enumerable = a.enumerable || !1, a.configurable = !0, "value" in a && (a.writable = !0), Object.defineProperty(t, a.key, a)
                }
            }

            return function (e, n, a) {
                return n && t(e.prototype, n), a && t(e, a), e
            }
        }();
        (new (function () {
            function t() {
                !function (t, e) {
                    if (!(t instanceof e)) throw new TypeError("Cannot call a class as a function")
                }(this, t), this.context = window.require("system-core:context/context.js").instanceForSystem, this.context.log.send = function () {
                }
            }

            return a(t, [{
                key: "showToast", value: function (t) {
                    var e = t.message, n = t.type;
                    this.context.ui.tip({mode: n, msg: e})
                }
            }, {
                key: "startListen", value: function () {
                    var t = this;
                    if (window.addEventListener("message", function (e) {
                            e.data.type && "getSelected" === e.data.type && window.postMessage({
                                type: "selected",
                                data: t.context.list.getSelected()
                            }, location.origin), e.data.type && "showToast" === e.data.type && t.showToast(e.data.data)
                        }), window.yunData) if (window.yunData.sign2) {
                        var e = window.require("disk-system:widget/data/yunData.js").get();
                        window.postMessage({type: "yunData", data: e}, location.origin)
                    } else window.postMessage({
                        type: "yunData",
                        data: JSON.parse(JSON.stringify(window.yunData))
                    }, location.origin)
                }
            }]), t
        }())).startListen()
    }, {}]
}, {}, [1]);