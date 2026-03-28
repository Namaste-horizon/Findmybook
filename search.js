const demoBooks = [
    {title:"Data Structures"},
    {title:"Algorithms"}
];

function searchBooks(){
    const container = document.getElementById('results');
    container.innerHTML = '';

    demoBooks.forEach(book=>{
        const div = document.createElement('div');
        div.className='result-card';

        div.innerHTML = `
        <h3>${book.title}</h3>
        <button onclick="rentBook('${book.title}')">Rent</button>
        `;

        container.appendChild(div);
    });
}