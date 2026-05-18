function colorUsernameByRating(rating) {
	const value = Number(rating);
	if (Number.isNaN(value)) {
		return null;
	}
	if (value >= 3000) {
		return null;
	}
	if (value >= 2600) {
		return "red";
	}
	if (value >= 2400) {
		return "red";
	}
	if (value >= 2300) {
		return "orange";
	}
	if (value >= 2100) {
		return "orange";
	}
	if (value >= 1900) {
		return "violet";
	}
	if (value >= 1600) {
		return "blue";
	}
	if (value >= 1400) {
		return "cyan";
	}
	if (value >= 1200) {
		return "green";
	}
	return "gray";
}