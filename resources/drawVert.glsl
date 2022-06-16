#version 330 core
in vec2 center;
void main(void){
        vec4 pos = proj*vec4(540.0*X.x,540.0*X.y,0.0,1.0);
        center = a_position.xy;
        gl_Position = vec4(a_position.xy+pos.xy)
        gl_PointsSize = 8.0;
}
