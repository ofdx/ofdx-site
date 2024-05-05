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
			onload:     for XMLHttpRequest.onload
			success:    when readyState = 4 and http.status = 200
			unhandled:  when readyState = 4 and status != 200
			completion: when readyState = 4 (after success/unhandled)
			onerror:    for XMLHttpRequest.onerror
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

			if(http.readyState === 4){
				switch(http.status){
					case 200:
						if(meta.success)
							meta.success(http);
						break;
					default:
						if(meta.unhandled)
							meta.unhandled(http);
				}

				if(meta.completion)
					meta.completion(http);
			}
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
