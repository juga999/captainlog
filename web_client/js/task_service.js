(function() {

window.CaptainLog.TaskService = {
    getInfo: function() {
        return fetch("/api/info").then(response => response.json());
    },
};
    
})();