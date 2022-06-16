varying vec2 centre;
out vec4 colour;
void main(void){
        vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
        float dd = length(circCoord);
        float alpha = 1.0-smoothstep(0.9,1.1,dd);
        colour = vec4(1.0,0.0,0.0,alpha);
        if (colour.a == 0.0){
                discard;
        }
}
