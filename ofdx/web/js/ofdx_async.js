/*
	XMLHttpRequest wrapper
	mperron (2023)
*/

var OfdxAsync = {
	method: undefined,
	target: undefined,
	
	get: function(meta){
		if(!meta)
			return false;

		var http = new XMLHttpRequest();
		this.method = (meta.method) ? meta.method : "GET";
		this.target = meta.target;

		http.onreadystatechange = function(){
			if(meta.onreadystatechange)
				return meta.onreadystatechange(http);

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
		}

		var post = (this.method === "POST") ? meta.post : undefined;
		http.open(this.method, this.target, true);
		http.send(post);
	},
};
