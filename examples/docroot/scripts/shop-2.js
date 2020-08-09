/*
	simple shopping cart example
*/

class ShoppingCart {
	constructor(cart) {
		this.cartContent = cart;
		this.cartContent.items = [];

		const accountForm = document.getElementById('account-form');
		accountForm.style.display = 'none';

		const shoppingPage = document.getElementById('shopping-page');
		shoppingPage.style.display = 'unset';

		document.getElementById('user-name').textContent = name;

		[...document.getElementsByClassName('shopping-item')]
			.forEach(item => {
				item.addEventListener('click', (evt) => {
					evt.preventDefault();
					this.addToCart(item.dataset.item);
				})
			});
	}

	addToCart(item) {
		const ix = this.cartContent.items.findIndex((i) => i.name === item);
		if (ix < 0)
			this.cartContent.items.push({ name: item, count: 1 });
		else
			this.cartContent.items[ix].count += 1;
		this.updateOrder();
	}

	deleteCartItem(item) {
		const ix = this.cartContent.items.findIndex((i) => i.name === item);
		if (ix < 0)
			this.cartContent.items.push({ name: item, count: 1 });
		else if (this.cartContent.items[ix].count == 1)
			this.cartContent.items.splice(ix, 1);
		else
			this.cartContent.items[ix].count -= 1;

		this.updateOrder();
	}

	updateOrder() {
		fetch(`/cart/${this.cartContent.cartID}`, {
			method: "PUT",
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify(this.cartContent)
		})
		.then(r => {
			if (! r.ok)
				throw 'error';

			const cartListContainer = document.getElementById('cart-list-container');
			
			if (this.cartContent.items.length == 0)
				cartListContainer.style.display = 'none';
			else
			{
				cartListContainer.style.display = 'initial';

				const cartList = document.getElementById('cart-list');
				[...cartList.querySelectorAll('li:not(:first-child)')]
					.forEach(li => li.remove());
				
				const li = cartListContainer.querySelector('li');

				this.cartContent.items.forEach(item => {
					const lic = li.cloneNode(true);
					lic.querySelector('span.text-placeholder').textContent = `${item.name} (${item.count})`;
					const removeBtn = lic.querySelector('span.cart-item');
					removeBtn.addEventListener('click', (li) => this.deleteCartItem(item.name));
					cartList.append(lic);
				});
			}
		})
		.catch(err => {
			console.log(err);
			alert('Failed to add item to cart');
		});
	}
};

window.addEventListener('load', () => {
	const createCartBtn = document.getElementById('create-cart-btn');
	createCartBtn.addEventListener('click', (evt) => {
		evt.preventDefault();
		
		const client = document.forms['client-form']['client-name'].value;

		if (client == "")
			alert("Please enter a user name");
		else
		{
			const newCart = {
				client: client
			};

			fetch('/cart', {
					method: "POST",
					headers: {
						'Content-Type': 'application/json'
					},
					body: JSON.stringify(newCart)
				})
				.then(r => r.json())
				.then(cartID => {
					newCart.cartID = cartID;
					new ShoppingCart(newCart);
				})
				.catch(err => {
					console.log(err);
					alert("failed to create shopping cart");
				})
		}
	});
});