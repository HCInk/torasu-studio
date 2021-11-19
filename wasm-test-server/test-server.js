const express = require('express');
const app = express();

app.use((_, res, next) => {
  res.header('Cross-Origin-Opener-Policy', 'same-origin');
  res.header('Cross-Origin-Embedder-Policy', 'require-corp');
  next();
});

if (process.argv.length > 2) {
	app.use(express.static(process.argv[2]));
} else {
	app.use(express.static('.'));
}

const PORT = process.env.PORT || 8080;

app.listen(PORT, () => {
  console.log(`Server listening on port ${PORT}...`);
}); 
