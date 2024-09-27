fn FizzBuzz() {
  for(let i = 0; i <= 100; i++) {
    
    if(i % 5 == 0 and i % 3 == 0){
      write("FizzBuzz");
    } elif(i % 5 == 0) {
      write("Buzz");
    } elif(i % 3 == 0) {
      write("Fizz");
    }
  }
}
