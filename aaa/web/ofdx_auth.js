/*
   OFDX authentication
   mperron (2023)
*/

window.addEventListener("load", (e) => {
	var form_login = document.forms['ofdx_login'];
	var inFlight = false;

	if(form_login){
		form_login.addEventListener('submit', (event) => {
			event.preventDefault();

			// Prevent double-submission of the form while the response is pending.
			if(inFlight)
				return;

			inFlight = true;
			for(let i = 0; i < form_login.elements.length; ++i)
				form_login.elements[i].disabled = true;

			OfdxAsync.send({
				target: form_login.action,
				method: 'POST',
				auth: {
					user: form_login.elements['ofdx_user'].value,
					pass: form_login.elements['ofdx_pass'].value
				},
				ondone: (http) => {
					if(http.status == 204){
						let redir = form_login.elements['ofdx_redir'];

						if(redir)
							window.location = redir;
						else
							window.location.reload();
					} else alert(http.response);

					// Release the form controls so the user can try again.
					inFlight = false;
					for(let i = 0; i < form_login.elements.length; ++i)
						form_login.elements[i].disabled = false;
				}
			});
		});
	}
});
