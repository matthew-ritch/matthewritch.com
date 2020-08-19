let myImage = document.querySelector('img');

myImage.onclick = function() {
    let mySrc = myImage.getAttribute('src');
    if(mySrc === 'images/user-image.png') {
      myImage.setAttribute('src','https://i5.walmartimages.com/asr/87061092-70de-4a9b-ae77-b0f20a9c54d5_1.520c5864c2391e9f24041a1faa3d5d3a.jpeg?odnWidth=612&odnHeight=612&odnBg=ffffff');
    } else {
      myImage.setAttribute('src','images/user-image.png');
    };
};

