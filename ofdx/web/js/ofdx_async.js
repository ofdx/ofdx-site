/*
	XMLHttpRequest wrapper
	mperron (2024)
*/

var OfdxAsync = {
	/*
		Send an async request. Argument should be an object:
		{
			method:    request method (default GET)
			post:      optional body for PUT/POST
			target:    target URL
			auth:      basic auth credentials as object: { user, pass }
			type:      XMLHttpRequest.responseType (e.g. 'json')

			// Callback functions
			onload:  for XMLHttpRequest.onload
			ondone:  when readyState = 4
			onerror: for XMLHttpRequest.onerror
		}
	*/
	send: function(meta){
		if(!meta)
			return false;

		let http = new XMLHttpRequest();

		http.responseType = meta.type || undefined;
		http.open((meta.method || "GET"), meta.target, true);
		http.onload = function(xhrEvent){
			if(meta.onload)
				return meta.onload(http, xhrEvent);

			if((http.readyState === 4) && meta.ondone)
				meta.ondone(http);
		};
		http.onerror = function(xhrEvent){
			if(meta.onerror)
				return meta.onerror(http, xhrEvent);

			console.log(http.statusText);
		};

		if(meta.auth){
			http.setRequestHeader(
				'Authorization',
				('Basic ' + btoa(meta.auth.user + ':' + meta.auth.pass))
			);
		}

		http.send(meta.post);
	},
};
