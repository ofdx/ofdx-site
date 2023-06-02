/*
   OFDX authentication
   mperron (2023)
*/

window.addEventListener("load", (e) => {
	var form_login = document.forms['ofdx_login'];
	var inFlight = false;

	if(form_login){
		// FIXME debug
		console.log('login form detected');

		form_login.addEventListener('submit', (event) => {
			event.preventDefault();
			if(inFlight)
				return;

			inFlight = true;
			for(let i = 0; i < form_login.elements.length; ++i)
				form_login.elements[i].disabled = true;

			var ofdx_user = form_login.elements['ofdx_user'];
			var ofdx_pass = form_login.elements['ofdx_pass'];

			// FIXME debug
			console.log('form submitted:', form_login);

			OfdxAsync.send({
				target: form_login.action,
				method: 'POST',
				auth: {
					user: ofdx_user.value,
					pass: ofdx_pass.value
				},
				completion: (http) => {
					// FIXME debug
					console.log(http);

					inFlight = false;
					for(let i = 0; i < form_login.elements.length; ++i)
						form_login.elements[i].disabled = false;
				}
			});
		});
	}
});
