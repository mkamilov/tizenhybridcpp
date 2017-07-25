/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*global tizen, tau, window, document, history, console*/

// strict wrapper
(function mainWrapper() {
    'use strict';

    /**
     * Service application id.
     *
     * @const {string}
     */
    var SERVICE_APP_ID = 'lEge4M91Nz.hybridservice',

        /**
         * Service port name.
         *
         * @const {string}
         */
        SERVICE_PORT_NAME = 'SAMPLE_PORT',

        /**
         * Local message port name.
         *
         * @const {string}
         */
        LOCAL_MESSAGE_PORT_NAME = 'SAMPLE_PORT_REPLY',

        /**
         * Local message port object.
         *
         * @type {LocalMessagePort}
         */
        localMessagePort = null,

        /**
         * Remote message port object.
         *
         * @type {RemoteMessagePort}
         */
        remoteMessagePort = null,

        /**
         * Local message port listener id that can be used later
         * to remove the listener.
         *
         * @type {number}
         */
        localMessagePortWatchId = null,

        /**
         * Indicates whether the message port is during start-up.
         *
         * @type {boolean}
         */
        isStarting = false,

        /**
         * Launches hybrid service application.
         *
         * @type {function}
         */
        launchServiceApp = null,

        /**
         * Start button element.
         *
         * @type {HTMLElement}
         */
        startBtn = document.getElementById('btn-start'),

        /**
         * Stop button element.
         *
         * @type {HTMLElement}
         */
        stopBtn = document.getElementById('btn-stop'),

        /**
         * Logs list element.
         *
         * @type {HTMLElement}
         */
        logsListEl = document.getElementById('logs'),

        /**
         * Alert popup.
         *
         * @type {HTMLElement}
         */
        alertPopup = document.getElementById('alert-popup');

    /**
     * Writes a message on the screen.
     *
     * @param {string} message
     */
    function writeToScreen(message) {
        var today = new Date(),
            time = today.getFullYear() + '-' + (today.getMonth() + 1) + '-' +
                today.getDate() + ' ' + today.getHours() + ':' +
                today.getMinutes() + ':' + today.getSeconds() + '.' +
                today.getMilliseconds(),
            newElement = document.createElement('LI');

        newElement.classList.add('ul-li-static');
        newElement.innerHTML = message + '<div class="li-text-sub">' +
            time + '</div>';

        logsListEl.appendChild(newElement);
    }

    /**
     * Sends message to another application.
     *
     * @param {string} command
     */
    function sendCommand(command) {
    	console.log("send command " + command);
        try {
            remoteMessagePort.sendMessage([{
                key: 'command',
                value: command
            }],
                localMessagePort);
            writeToScreen('Sending: ' + command);
        } catch (error) {
            console.error(error);
        }
    }

    /**
     * Performs action after receiving message from another application.
     *
     * @param {MessagePortDataItem[]} data
     */
    function onReceive(data) {
        var message = null,
            i = 0,
            len = data.length;

        for (i = 0; i < len; i += 1) {
            if (data[i].key === 'server') {
                message = data[i].value;
            }
        }

        writeToScreen('Received : ' + message);

        if (message === 'WELCOME') {
            sendCommand('start');
        } else if (message === 'stopped') {
            sendCommand('exit');
        } else if (message === 'exit') {
            if (remoteMessagePort) {
                remoteMessagePort = null;
            }
            if (localMessagePort) {
                try {
                    localMessagePort
                        .removeMessagePortListener(localMessagePortWatchId);
                    localMessagePort = null;
                } catch (error) {
                    console.error(error);
                }
            }
        }
    }

    /**
     * Initializes message port.
     */
    function startMessagePort() {
        try {
            localMessagePort = tizen.messageport
                .requestLocalMessagePort(LOCAL_MESSAGE_PORT_NAME);
            localMessagePortWatchId = localMessagePort
                .addMessagePortListener(function onDataReceive(data, remote) {
                    onReceive(data, remote);
                });
        } catch (e) {
            localMessagePort = null;
            console.log("exception in requestlocalport");
            writeToScreen(e.name);
        }
        console.log("done with local ports");

        try {
            remoteMessagePort = tizen.messageport
                .requestRemoteMessagePort(SERVICE_APP_ID, SERVICE_PORT_NAME);
        } catch (ex) {
            remoteMessagePort = null;
            console.log("exception in requestremoteport");
            writeToScreen(ex.name);
        }

        isStarting = false;

        sendCommand('connect');
    }

    /**
     * Displays popup with alert message.
     *
     * @param {string} message
     */
    function showAlert(message) {
        alertPopup.querySelector('#message').innerHTML = message;
        tau.openPopup(alertPopup);
        alertPopup.addEventListener('click', function onClick() {
            tau.closePopup(alertPopup);
        });
    }

    /**
     * Performs action when getAppsContext method of tizen.application API
     * results in error.
     *
     * @param {Error} err
     */
    function onGetAppsContextError(err) {
        console.error('getAppsContext exc', err);
    }

    /**
     * Performs action on a list of application contexts
     * for applications that are currently running on a device.
     *
     * @param {ApplicationContext[]} contexts
     */
    function onGetAppsContextSuccess(contexts) {
        var i = 0,
            len = contexts.length,
            appInfo = null;

        for (i = 0; i < len; i = i + 1) {
            try {
                appInfo = tizen.application.getAppInfo(contexts[i].appId);
            } catch (exc) {
                console.error('Exception while getting application info: ' +
                    exc.message);
                showAlert('Exception while getting application info:<br>' +
                    exc.message);
            }

            console.log("checking app with id " + appInfo.id);
            if (appInfo.id === SERVICE_APP_ID) {
            	console.log("found service id " + SERVICE_APP_ID);
                break;
            }
        }

        
        if (i >= len) {
            console.log("launching service app");
            launchServiceApp();
        } else {
            console.log("starting message port");
            startMessagePort();
        }
    }

    /**
     * Starts obtaining information about applications
     * that are currently running on a device.
     */
    function start() {
        try {
            tizen.application.getAppsContext(onGetAppsContextSuccess,
                onGetAppsContextError);
        } catch (e) {
            writeToScreen('Get AppContext Error: ' + e.message);
        }
    }

    /**
     * Launches hybrid service application.
     */
    launchServiceApp = function launchServiceApp() {
        function onSuccess() {
            start();
        }

        function onError(err) {
            console.error('Service Applaunch failed', err);
            isStarting = false;
            showAlert('Failed to launch HybridServiceApp!');
        }

        try {
            tizen.application.launch(SERVICE_APP_ID, onSuccess, onError);
        } catch (exc) {
            console.error('Exception while launching HybridServiceApp: ' +
                exc.message);
            showAlert('Exception while launching HybridServiceApp:<br>' +
                exc.message);
        }
    };

    /**
     * On click on start button handler.
     */
    function onStartBtnTap() {
        startBtn.classList.add('hidden');
        stopBtn.classList.remove('hidden');

        if (localMessagePort) {
            showAlert('Cannot start:<br>already running');
        } else if (isStarting) {
            showAlert('Cannot start:<br>service is starting');
        } else {
            isStarting = true;
            start();
        }
    }

    /**
     * On click on stop button handler.
     */
    function onStopBtnTap() {
        stopBtn.classList.add('hidden');
        startBtn.classList.remove('hidden');

        console.log("stop button clicked");
        if (isStarting) {
            showAlert('Cannot stop:<br>service is starting');
        } else if (remoteMessagePort) {
            sendCommand('stop');
        } else {
            showAlert('Cannot stop:<br>not running');
        }
    }

    /**
     * On click on clear button handler.
     */
    function onClearBtnTap() {
        logsListEl.innerHTML = '';
    }

    /**
     * Creates TAU Selector and bind it's events.
     */
    function bindSelectorEvents() {
        var page = document.querySelector('#main'),
            elSelector = page.querySelector('#selector'),
            handler = page.querySelector('.popup-handler'),
            popup = page.querySelector('#selector-popup'),
            selector = null;

        function clickHandler() {
            tau.openPopup(popup);
        }

        page.addEventListener('pagebeforeshow', function onPageBeforeShow() {
            // selector items radius
            var radius = window.innerHeight / 2 * 0.8;

            // checks if TAU supports circle shape
            if (tau.support.shape.circle) {
                handler.addEventListener('click', clickHandler);
                // creates new TAU Selector
                selector = tau.widget.Selector(elSelector, {
                    itemRadius: radius
                });
            }
        });

        page.addEventListener('pagebeforehide', function onPageBeforeHide() {
            // checks if TAU supports circle shape
            if (tau.support.shape.circle) {
                handler.removeEventListener('click', clickHandler);
                selector.destroy();
            }
        });

        elSelector.addEventListener('click', function onSelectorClick() {
            // only available option in Selector is clear
            onClearBtnTap();
            tau.closePopup(alertPopup);
        });
    }

    /**
     * Binds events.
     */
    function bindEvents() {
        startBtn.addEventListener('click', onStartBtnTap);
        stopBtn.addEventListener('click', onStopBtnTap);

        window.addEventListener('tizenhwkey', function onTizenHWKey(e) {
            if (e.keyName === 'back') {
                if (tau.activePage.id === 'main') {
                    try {
                        tizen.application.getCurrentApplication().exit();
                    } catch (exc) {
                        console.error('Error: ', exc.message);
                    }
                } else {
                    history.back();
                }
            }
        });

        bindSelectorEvents();
    }

    /**
     * Initializes main module.
     */
    function initMain() {
        bindEvents();
    }

    initMain();
})();
