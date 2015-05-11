all: mandelbrot paren_match

mandelbrot:
	nvcc mandelbrot.cu -o mandelbrot

paren_match:
	nvcc paren_match.cu -o paren_match

clean:
	rm -f mandelbrot paren_match

.PHONY: all mandelbrot paren_match clean
