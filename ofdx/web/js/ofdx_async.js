/*
	XMLHttpRequest wrapper
	mperron (2023)
*/

var OfdxAsync = {
	method: undefined,
	target: undefined,
	
	send: function(meta){
		if(!meta)
			return false;

		var http = new XMLHttpRequest();
		this.method = (meta.method) ? meta.method : "GET";
		this.target = meta.target;

		var post = (this.method === "POST") ? meta.post : undefined;

		http.open(this.method, this.target, true);
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

		http.send(post);
	},
};
